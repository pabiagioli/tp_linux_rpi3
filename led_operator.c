#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <jansson.h>

typedef struct led_ops {
    char* led;
    char* ledDir;
    char* ledTriggers;
} led_object_t;

typedef struct led_obj_list {
    led_object_t* leds;
    int listSize;
} led_list_t;

/**
 * GET TRIGGERS FROM TEXT FILE ON DEVICE FOLDER
 * --------------------------------------------
 * device Path (directory): 
 * ------------------------
 * /sys/devices/platform/leds/leds/led1
 * 
 * trigger file: 
 * -------------
 * /sys/devices/platform/leds/leds/led1/trigger
 * 
 * */
char* getTriggers(char *devicePath){
    struct stat fileStats;
    DIR *d;
    size_t nread;
    char buf;
    char triggerFilePath [PATH_MAX]; 
    sprintf(triggerFilePath, "%s/trigger", devicePath);
    
    stat(triggerFilePath,&fileStats);

    char *result = malloc(fileStats.st_size + 1);
    int ptr = 0;


    FILE *fd = fopen(triggerFilePath, "r");
    if(fd){
        while ((nread = fread(&buf, 1, 1, fd)) > 0) {
            if(buf != '\n'){
                result[ptr] = buf;
                ptr++;
            }
                
        }
        result[ptr] = '\0';
        fclose(fd);
    }

    return result;
}

/**
 * GET LEDS
 * --------
 * 
 * */
led_list_t getLeds (const char *ledFolder){
    led_list_t result;
    led_object_t *ledObjs;// = malloc(sizeof(led_object_t));
    DIR *dp;
    struct dirent *ep;
    struct stat ledStat;
    char *myFileName;
    char tmpPath [PATH_MAX], ledRelPath [PATH_MAX], *ledCanonicalPath;
    ssize_t len;
    int totalLeds = 0;

    dp = opendir (ledFolder);
    if (dp) {
        while( ep = readdir(dp)){
            if(strcmp(ep->d_name,".") && strcmp(ep->d_name,"..")){
                totalLeds++;
            }
        }
        rewinddir(dp);
        ledObjs = malloc(sizeof(led_object_t)* totalLeds);
        totalLeds = 0;
        while (ep = readdir (dp)){
            myFileName = malloc(255);
            sprintf(myFileName, "%s", ep->d_name);
            //myFileName = ep->d_name;
            if(strcmp(myFileName,".") && strcmp(myFileName,"..")){
                sprintf(tmpPath,"%s/%s", ledFolder, myFileName); //tmpPath = ledFolder + "/" + myFileName;
                lstat(tmpPath, &ledStat);
                if(S_ISLNK(ledStat.st_mode)){
                    if ((len = readlink(tmpPath, ledRelPath, PATH_MAX-1)) != -1){
                        ledRelPath[len] = '\0'; //ledRelPath = ../../devices...
                        sprintf(tmpPath, "%s/%s", ledFolder, ledRelPath);
                        ledCanonicalPath = malloc(PATH_MAX);

                        char *ptr = realpath(tmpPath, ledCanonicalPath);
                        if(ptr){
                            ledObjs[totalLeds] = (led_object_t) { 
                                .led = myFileName, 
                                .ledDir = ptr, 
                                .ledTriggers = getTriggers(ledCanonicalPath) };
                        }
                    }
                }
                totalLeds++;
            }
            
        }
        (void) closedir (dp);
    } else {
        perror ("Couldn't open the directory");
    }
    result = (led_list_t){.leds = ledObjs, .listSize = totalLeds};
    return result;
}

int min(int a, int b){
    if (a < b)
        return a;
    if (b < a)
        return b;
}


void changeTrigger(char *led, char *trigger) {
    
    char triggerFilePath [PATH_MAX]; 
    struct stat triggerFilePathStat;
    sprintf(triggerFilePath, "/sys/devices/platform/leds/leds/%s/%s",led,"trigger");
    //sprintf(triggerFilePath, "echo %s > /sys/devices/platform/leds/leds/%s/%s",trigger, led,"trigger");
    FILE *fd = fopen(triggerFilePath, "w");
    fputs(trigger,fd);
    fflush(fd);
    fclose(fd);
    //execute a shell command (For extreme USERS)
    //system(triggerFilePath);
}

int main(int argc, char **argv) {

    json_t *led_array;
    json_t *triggers;

    char *dirArg;
    led_list_t list;
    int i = 0;

    char *httpMethod = getenv("REQUEST_METHOD");
    char *queryString;
    
    if(httpMethod && strcmp(httpMethod,"GET") == 0){
        queryString = getenv("QUERY_STRING");
        //parse query string for the dirArg;
        if(queryString && strlen(queryString) > 0)
            dirArg = queryString;
        else    
            dirArg = "/sys/class/leds";
    }else if (httpMethod && strcmp(httpMethod,"POST") == 0){
        char Buffer[512]={0};
        int InputLength = atoi( getenv("CONTENT_LENGTH") );
        InputLength = min( InputLength, sizeof(Buffer)-1 ); /* Avoid buffer overflow */
        fread( Buffer, InputLength, 1, stdin );
        json_t *request = json_loads(Buffer,JSON_DECODE_ANY, NULL);
        /* POST variables */
        char *ledStr, *triggerStr;
        //json_unpack(request, "{s:s,s:s}", "led", ledStr, "trigger", triggerStr);
        ledStr = (char *) json_string_value(json_object_get(request,"led"));
        triggerStr = (char *) json_string_value(json_object_get(request,"trigger"));
        
        changeTrigger(ledStr, triggerStr);

        printf("Content-type: text/html\n\n");
        printf("%s %s \n",ledStr, triggerStr);
        free(request);
        return 0;

    }else{
        if(argc > 1)
            dirArg = argv[1];
        else
            dirArg = "/sys/class/leds";
    }

    list = getLeds(dirArg);

    led_array = json_array();
    
    for(i = 0; i< list.listSize; i++){
        //printf("led = %s , dir = %s, triggers = %s \n", list.leds[i].led, list.leds[i].ledDir, list.leds[i].ledTriggers);
        json_auto_t *ledObj = json_object();
        json_object_set(ledObj, "led", json_string(list.leds[i].led));
        json_object_set(ledObj, "triggers", json_string(list.leds[i].ledTriggers));
        json_array_append(led_array, ledObj);
        //json_decref(ledObj);
    }
    //json_decref(led_array);
    //char *output = json_dumps(led_array, JSON_INDENT(4));
    char *output = json_dumps(led_array, JSON_COMPACT);
    printf("Content-type:application/json\n\n");
    printf("%s \n", output);
    free(output);

    return 0;

}