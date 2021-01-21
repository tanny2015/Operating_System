#include <stdio.h>
#include <unistd.h> 
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h> 
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
// #include <glob.h>
#include "list.h"
// #include <wordexp.h>
#include <libgen.h>
 
#define firstAccessTypeBrief 1
#define firstAccessTypeDetailed 2

// #define isDir 1
// #define isFile 2

static List *topLevelDirs;
static List *topLevelFiles;
static List *topLevelSymbolicDirs;
void freeLists();
void printAllLists();
char workingDir[PATH_MAX];//store current working dir
//only one thread, it is thread safe. 
//can be used repeatedly.
char printPaperForPrintQuotes[PATH_MAX];
char printPaperForFilter[PATH_MAX];
//keys
static bool first_access_in_a_node_traverse = true;
static bool I_Key = false;
static bool L_Key = false;
static bool R_Key = false;

//main implementations
void printCenter();
void generateTopDirs(char* path);
void showL(char *pathStr , List* pool);
void showDirname(char * pathStr , List* pool);
void printFileBriefInfo(char *folderName, char* filename);
void printFileDetailInfo(char *folderName, char* filename);


//validation

bool canOpenLink(char* path);
bool validFlagPara(char *flag, int startpoint);//set keys as well

//trivial
char* blankEscape(char * originalStr);//string handling
bool shouldAddQuotes(char *str);
int isFile(char *path);
int isDir(char *path);
int isSymbolicLinkToDir(char* path);
int isSymbolicLink(char *path);
char* filteredFilenameOrDir(char *fileOrDir);
// char* realPath(char *fileOrDir);
void printPermission(struct stat buf);
void singleFileHandling(char *pathStr,int accessType);
void printTime(struct stat buf);
void softlinkPrinting(char*filenameOrDir);
void setKeys(bool I, bool L, bool R);
void printList(List * list, char* listname);//for debug
void releaseItem(void * pItem);


//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  ENTRY   /////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

//[arg0--filename][arg1-- ][arg2- ][arg3- ]
int main(int argc, char** args) {  

    topLevelDirs = List_create(); 
    topLevelFiles = List_create();
    topLevelSymbolicDirs = List_create();
    getcwd(workingDir, sizeof(workingDir));

    //1.there is no flag parameter or dir input
    //R=F | l=F | i=F | dir = '.'
    if(argc == 1){
        setKeys(false,false,false);
        generateTopDirs(".");
        printAllLists();
        freeLists();
        return EXIT_SUCCESS;
    } 
    int flagIt = 1;
    for(flagIt = 1; flagIt < argc; flagIt++){ 
        char* flag = args[flagIt];  
        //1.flag parameters[always in front of dir paramaters] 
        if(flag[0] == '-'){
            //check the flag's validity, if so,set I/R/L_Key, continue
            //if not valid, then exit the program(handled in the sub func)
            if(validFlagPara(flag, 1)){
                continue;
            }
        }else{
            break;
        } 
    } 
    //2.there is no flag parameter, have to set Keys manually
    if(flagIt == 1){
        setKeys(false,false,false);
    }

    //3.there is no directory parameter | dir = '.'
    if(flagIt == argc ){
        generateTopDirs(".");
        printAllLists();
        freeLists();
        return EXIT_SUCCESS;
    }
    

    //4.there is at least 1 directory parameter
    int dirIt = flagIt;
    for( dirIt = flagIt; dirIt < argc; dirIt++){ 
        // ls: cannot access ' .': No such file or directory
        // char* dir = trimwhitespace(args[it]); //should not do trim!
        char* dir = args[dirIt];  
        //printf("%s  -- length: %ld \n", dir, strlen(dir));
        //printf("%s  -- argcount: %d \n" , dir, argc);
        generateTopDirs(dir); 
    } 

    printAllLists();
    freeLists(); 
}


//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////  Main Implemetation   //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void printCenter(List *fileOrDir){
    if(List_count(topLevelFiles) + List_count(topLevelDirs) + List_count(topLevelSymbolicDirs) == 0){
        printf("there is no valid file or dir, program terminates now!  \n");
        freeLists();
        exit(EXIT_SUCCESS);
    }
    // move cursor to the first to facilitate iteration
    char *topLevel = (char *)List_first(fileOrDir); 
    if(!topLevel){
        //printf("NO valid directory link! please check! \n");
        return;
    }

    while(topLevel){
        first_access_in_a_node_traverse = true; 

        if(L_Key == true){
            showL(topLevel, fileOrDir);
        }
        else{
            showDirname(topLevel, fileOrDir);
        }
        
        topLevel = List_next(fileOrDir);
    }
    //puts("");
}
 

void generateTopDirs(char* path){
    //printf("%s ============= \n", path); 
    //1.wordexp() will split at the emtpy space, in order to keep the space, we need to escape by '\ ' 
    //char* newPath = blankEscape(path);

    //decide not to get real full path -- caz assignment requirement needs us to display the original paths
    //https://stackoverflow.com/questions/2341808/how-to-get-the-absolute-path-for-a-given-relative-path-programmatically-in-linux
    /*realpath(newPath, fullRealPath);*/

    //decide not to use 'glob function' for filter handling(for filters, '/home/*as*4/...) caz it will not catch tilde(~)
    //printf("%s  ++ length: %ld \n", newPath, strlen(newPath));

    //decide not ot use 'wordexp() ' caz it can not support brackets 
    if(canOpenLink(path)){
        //printf("%s  --@@ length: %ld \n", *linkP, strlen(*linkP));
        //move the cursor to the first item
        char *originalPath = malloc(sizeof(char) * PATH_MAX);
        strcpy(originalPath, path);

        //char temp[PATH_MAX];  *** dont do this, it will make weird bug![can only add one item!] 
        //decide not to check duplicate in the list, caz original 'ls' will not filter out the duplicate neither. 
        
        //if the symbolic refer to dir, it will be treated as a folder, else, it will be treated as regular file
        if(isFile(originalPath)){
            List_last(topLevelFiles);
            List_append(topLevelFiles, originalPath);  
        }else if(isSymbolicLink(originalPath) && isSymbolicLinkToDir(originalPath)){
            List_last(topLevelSymbolicDirs);
            List_append(topLevelSymbolicDirs, originalPath);  
        }else if(isDir(originalPath)){
            List_last(topLevelDirs);
            List_append(topLevelDirs, originalPath);  
        }else{
            //for debug
            printf("we will only handle (regular | dir | symbolic link) file! \n");
            ;
        }
    }
    
    else{
        printf("the Link: <%s> is invalid and can not be processed! Program terminated now.. \n", filteredFilenameOrDir(path));
         
        freeLists(); 
        exit(EXIT_SUCCESS);
    } 
   /*  printList(topLevelFiles, "topLevelFiles");//for debug 
    printList(topLevelDirs, "topLevelDirs");//for debug 
    printList(topLevelSymbolicDirs, "topLevelSymbolicDirs");//for debug */ 
     
}
 


void showL(char *pathStr, List* pool){ 
   
    if(pool == topLevelFiles || pool == topLevelSymbolicDirs){
        singleFileHandling(pathStr,firstAccessTypeDetailed);
        //first_access_in_a_node_traverse = false;
        //printf("\n");
        return;
    }

    //only 'dir' can go here!, in this showL() function, L_Key must be true
    //dir can be a 'symbolic folder'
    //if it is in one of the '-R' loop, if will never be a symbolic link(folder),so the only possibility is : it is the first time of the 'folder list' traverse
   
    DIR *dirStruct = opendir(pathStr);
    if(dirStruct == NULL){ 
        //open file failure
        printf("Open File Failed! <path = %s> \n", pathStr);
        closedir(dirStruct); 
        return;
    }


    if(first_access_in_a_node_traverse){
        if(List_count(topLevelFiles) + List_count(topLevelSymbolicDirs) > 0){
            printf("\n%s:\n", filteredFilenameOrDir(pathStr)); 
        }
        else if(List_count(topLevelDirs) > 1){
            printf("\n%s:\n", filteredFilenameOrDir(pathStr));
        }
        else if(List_count(topLevelDirs)> 0 && R_Key == true){
            printf("\n%s:\n", filteredFilenameOrDir(pathStr)); 
        } else{
            ;
        }
    }else{
        printf("\n%s:\n", filteredFilenameOrDir(pathStr)); 
    }

    first_access_in_a_node_traverse = false;
    
    
    

    //open file successfully
    //the system may has been malloc memory for dp. 
    //***dp has to be a pointer! and do NOT malloc memeory for it, do not apply 'memset' into,--segment fault!
    struct dirent *dp; 

    //On success, readdir() returns a pointer to a dirent structure.
    //(This structure may be statically allocated; do not attempt to free(3) it.) 
    //If the end of the directory stream is reached, NULL is returned and errno is not changed. 
    //If an error occurs, NULL is returned and errno is set appropriately.
    while((dp = readdir(dirStruct)) != NULL){ 
        //I will not handle . or .. or some hidden files 
        if (dp->d_name[0] == '.') { 
                continue; 
        }  
        else {
            printFileDetailInfo(pathStr,dp->d_name); 
        }  
    } 
    printf("\n" );
    closedir(dirStruct); 
    if(R_Key == true){
        struct dirent *dp; 
        DIR *dirStruct = opendir(pathStr);
        while((dp = readdir(dirStruct)) != NULL){
            if (dp->d_type == DT_DIR) { 
                if(dp->d_name[0] == '.'){
                    continue;
                } 

                char subPath[PATH_MAX];//next path
                if(pathStr[strlen(pathStr) - 1] == '/'){
                    snprintf(subPath, sizeof(subPath), "%s%s", pathStr, dp->d_name);
                }else{
                    snprintf(subPath, sizeof(subPath), "%s/%s", pathStr, dp->d_name);
                }
                //printf("\n"); 
                //printf("%s:\n", filteredFilenameOrDir(subPath)); 
                //if one can enter the following line, it must be a folder.
                showL(subPath,NULL); 
            }   
        } 
        closedir(dirStruct);
    }
    //closedir(dirStruct);
    //printf(" \n");
}



void showDirname(char * pathStr, List* pool){  
    if(pool == topLevelFiles){ 
        singleFileHandling(pathStr,firstAccessTypeBrief); 
        return;
    } 

    DIR *dirStruct = opendir(pathStr); 
    if(dirStruct == NULL){
        //open file failure 
        //perror("can not open!\n");
        printf("Open File Failed! <path = %s> \n", pathStr);
        closedir(dirStruct); 
        return;
    }
     
    
    if(first_access_in_a_node_traverse){
        if(List_count(topLevelFiles)> 0){
            printf("\n%s:\n", filteredFilenameOrDir(pathStr)); 
        }else if(List_count(topLevelSymbolicDirs) + List_count(topLevelDirs) > 1){
            printf("\n%s:\n", filteredFilenameOrDir(pathStr));
        }else if(List_count(topLevelSymbolicDirs) + List_count(topLevelDirs) > 0 && R_Key == true){
            printf("\n%s:\n", filteredFilenameOrDir(pathStr)); 
        }else{
            ;
        }
    }else{
        printf("\n%s:\n", filteredFilenameOrDir(pathStr)); 
    }

    first_access_in_a_node_traverse = false;
   
     
    struct dirent *dp; 

    //https://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux 
    while((dp = readdir(dirStruct)) != NULL){ 
        if (dp->d_name[0] == '.') {  
                continue; 
        }  
        else {
            printFileBriefInfo(pathStr,dp->d_name); 
        } 
    }

    printf("\n" );
    closedir(dirStruct); 
    if(R_Key == true){
        struct dirent *dp;
        //struct stat statBuffer; 
        DIR *dirStruct = opendir(pathStr);
        while((dp = readdir(dirStruct)) != NULL){
            if (dp->d_type == DT_DIR) { 
                if(dp->d_name[0] == '.'){
                    continue;
                } 
                char subPath[PATH_MAX];//next path
                if(pathStr[strlen(pathStr) - 1] == '/'){
                    snprintf(subPath, sizeof(subPath), "%s%s", pathStr, dp->d_name);
                }else{
                    snprintf(subPath, sizeof(subPath), "%s/%s", pathStr, dp->d_name);
                }
                
                //printf("\n");
                //printf("%s:\n", filteredFilenameOrDir(subPath)); 
                //if one can enter the following line, it must be a folder.
                showDirname(subPath,NULL); 
            }   
        } 
        closedir(dirStruct);
    } 
    //printf(" \n");
}

void singleFileHandling(char *pathStr,int accessType){
    
    /* if(List_count(topLevelDirs)>1 && isDir(pathStr)){
        printf("%s:\n", filteredFilenameOrDir(pathStr)); 
    }else{
        //printf("123456777 %d-----------%d\n",List_count(topDirs), isDir(pathStr));
    } */
    //if the root is a file(not a folder), not need to go further(even has -R) 
    //simply print the name
    if(accessType == firstAccessTypeBrief){
        // printf("%s ", filteredFilenameOrDir(pathStr)); 
        printFileBriefInfo(NULL,pathStr); 
        //printf("\n");
        //return;
    }
    else if(accessType == firstAccessTypeDetailed){
        // printf("%s ", filteredFilenameOrDir(pathStr));  
        printFileDetailInfo(NULL,pathStr); 
        //printf("\n");
    }else{
        printf("not handle this case! \n");
    }
    
}

//path is the parent dir of dp_name(current file)
//whether it is a file or dir, path should be a filename, like; test.c
//no need to transform it to a filename or dir. 
//if folderName is a NULL,then it is a file. otherwise, it is a folder
void printFileBriefInfo(char *folderName, char* filename){
    

    struct stat statBuffer; 
    memset(&statBuffer, 0,  sizeof(struct stat));

    char myPath[PATH_MAX];//next path
    memset(&myPath, 0,  PATH_MAX * sizeof(char)); 
    
    if(folderName){
        snprintf(myPath, sizeof(myPath), "%s/%s", folderName, filename); 
    }
    //**special case, where it is a file(not folder)
    else{
        //by default, the validity of a path in a list has been checked  before entering the list
        //realpath(folderName,myPath);
        strcpy(myPath, filename);
    }
    //printf("folderName = %s; newPath = %s;   \n", folderName, myPath );
    int status = lstat(myPath, &statBuffer);  
    if(status != 0){
        printf("stat handling failed! \n");
        return;
    } 

    int filenameIndent = 3;
    if(I_Key == true){
        printf("%lu ", statBuffer.st_ino);
    } else{
        filenameIndent = 0;
    }
 

    printf("%*s   ", filenameIndent,filteredFilenameOrDir(filename));
}


void printFileDetailInfo(char *folderName, char* filename){
    struct stat statBuffer; 
    memset(&statBuffer, 0,  sizeof(struct stat));

    char myPath[PATH_MAX];//next path
    memset(&myPath, 0,  PATH_MAX * sizeof(char));
    
    if(folderName){
        snprintf(myPath, sizeof(myPath), "%s/%s", folderName, filename);  
    }
    //**special case, where first_access=True && it is a file(not folder)
    else{
        //by default, the validity of a path in a list has been checked  before entering the list
        //realpath(path,myPath);
        strcpy(myPath, filename);
    }

    //printf("folderName = %s; newPath = %s;   \n", folderName, myPath );
    
    //file infos
    //lstat() is identical to stat(), except that if path is a symbolic link, then the link itself is stat-ed, not the file that it refers to.
    //returned value will be collected in the buffer
    int status = lstat(myPath, &statBuffer);  
    if(status != 0){
        printf("**stat handling failed! \n");
        return;
    }
    // display
    //nt nlinkIndent = 2;
    if(I_Key == true){
        printf("%lu   ", statBuffer.st_ino);
    } 

    // 1.permission
    printPermission(statBuffer); 

    // 2.The number of hard links to the file
    printf("%2lu", statBuffer.st_nlink);// (unsign long)

    // 3.The owner of the file (e.g. a1a1).
    struct passwd *ownerStruct = getpwuid(statBuffer.st_uid);
    printf("%10s   ", ownerStruct->pw_name);

    // 4.The name of the group the file belongs to (groups are collections of users, e.g. guest).
    struct group *groupStruct = getgrgid(statBuffer.st_gid);
    printf("%10s   ", groupStruct->gr_name);

    // 5.The size of the file in characters (bytes).
    printf("%10lu    ",statBuffer.st_size);

    // 6.The date and time of the most recent change to the contents of the file (e.g.May 10 15:21).
    // https://stackoverflow.com/questions/34080583/how-to-convert-monthint-into-month-name-string
    printTime(statBuffer);
    
    // 7.The name of the file or directory (e.g. banner.gif or document).
    if(S_ISLNK(statBuffer.st_mode)){
        softlinkPrinting(myPath);
    }
    else{
        printf("%s \n", filteredFilenameOrDir(filename));
    }
 
    
}

void softlinkPrinting(char* filenameOrDir){
    char softlinkBuffer[PATH_MAX];
    memset(softlinkBuffer, 0, PATH_MAX * sizeof(char));

    char rpathRaw[PATH_MAX];
    memset(rpathRaw, 0, PATH_MAX * sizeof(char));

    char rpath[PATH_MAX];
    memset(rpath, 0, PATH_MAX * sizeof(char));
    //rpath is the full dir right now /home/.../test.c
    //**do NOT use realpath, you can not go to another directory to read current symbolic link file
    //**stay in the directory where your symbolic link file is located(rather than go to its targeted file!!)
    //realpath(filenameOrDir,rpath);
    strcpy(rpathRaw, filenameOrDir);
    //dirname(path) will definitely change the value of the path
    //dirname(rpath); //now rpath become the parent dir /home/../parentDirOfTestC/
    strcpy(rpath, dirname(rpathRaw));
     


    char rbase[PATH_MAX+3];
    memset(rbase, 0, PATH_MAX * sizeof(char)); 
    //basename(path) might change the value of the path, but to gurentee the result, should using a new value
    strcpy(rbase, filenameOrDir);
    
    // printf("original rpath = %s, ", rpath);
    //printf("\n===  original = %s === rpath = %s == rbase = %s \n", filenameOrDir,rpath,rbase);
    
    //you need to change to file's parent dir, then you get this file's name. 
    //do NOT use readlink(basename(../dirname/filename)...) it can not work, because currently you are in the
    //parent dir, this time, '.' means this file's directory. so simply using readlink((filename)..) is correct.
    chdir(rpath);
    ssize_t resSize = readlink(basename(rbase),softlinkBuffer,PATH_MAX - 1);
    // printf("%s --> buffer ===  %ld--> buffersize ==== %ld --> resSize \n",softlinkBuffer, strlen(softlinkBuffer),resSize);
    if(resSize <= 0){
        printf("%s -> (**unknown: readlink error!) \n", filteredFilenameOrDir(filenameOrDir));
        perror("readlink");  
        chdir(workingDir);
        return;
    }else{ 
        //handle gibberish characters in Linux(乱码)
        //https://www.jianshu.com/p/2d1c0f13ddfb 
        char res[resSize];
        strncpy(res,softlinkBuffer, resSize); //copy n chars into new container
        res[resSize] = '\0'; 
        printf("%s -> %s \n", filteredFilenameOrDir(basename(rbase)), filteredFilenameOrDir(res));
        chdir(workingDir);
    }
    
}
//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////  Validation   //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

bool canOpenLink(char* path){
    struct stat temp;
    if(lstat(path, &temp) == 0){
        return true;
    }else{
        return false;
    }
}


bool validFlagPara(char *flag, int startpoint){
    for(int i = startpoint; i<strlen(flag); i++){
        if(flag[i]=='i'){
            I_Key = true;
        }
        else if(flag[i]=='R'){
            R_Key = true;
        }
        else if(flag[i]=='l'){
            L_Key = true;
        }
        else {
            printf("the input parameter:<%s> is invalid! please check! \n", flag);
            freeLists();
            exit(EXIT_SUCCESS);
        }
    }
    return true;
}

//pure dir, not a symbolic link
int isDir(char *path)
{
    struct stat path_stat;
    lstat(path, &path_stat);
    //printf("is dir = %d; is_file = %d, realpath = %s\n",S_ISDIR(path_stat.st_mode),S_ISREG(path_stat.st_mode),realpath(path,printPaperForFilter));
    
    return S_ISDIR(path_stat.st_mode);
}

 
//the following function should only handle two type: pointing to dir or regular file
//****THE parameter path have to be an symbolic path
int isSymbolicLinkToDir(char* path){
    if(!isSymbolicLink(path)){
        return 0;
    }
    //tell if the softlink point to dir or regular file
    //int is_dir = 0;
    char softlinkBuffer[PATH_MAX];
    memset(softlinkBuffer, 0, PATH_MAX * sizeof(char));

    char myPath[PATH_MAX];//next path
    memset(&myPath, 0,  PATH_MAX * sizeof(char));
    realpath(path,myPath);

    return isDir(myPath); 
}

int isFile(char *path)
{
    struct stat path_stat;
    lstat(path, &path_stat);
    //printf("is dir = %d; is_file = %d, realpath = %s\n",S_ISDIR(path_stat.st_mode),S_ISREG(path_stat.st_mode),realpath(path,printPaperForFilter));
    
     
    if(S_ISREG(path_stat.st_mode)){
        return 1;
    }

    if(S_ISLNK(path_stat.st_mode) && !isSymbolicLinkToDir(path)){
        return 1;
    } 

    return 0; 
}

int isSymbolicLink(char *path)
{
    struct stat path_stat;
    lstat(path, &path_stat);
    //printf("is dir = %d; is_file = %d, realpath = %s\n",S_ISDIR(path_stat.st_mode),S_ISREG(path_stat.st_mode),realpath(path,printPaperForFilter));
    return S_ISLNK(path_stat.st_mode); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////  Trivial   ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
 


//prof said if the filename or link contains non-alphanumeric number(the tail and head included)
//then you need to add the quotes on both sides of the filename or link
bool shouldAddQuotes(char *str){ 
    //str = "12123#123";
    int len = strlen(str);  
    for(int i = 0; i < len; i++){
        //printf("-- %c \n", str[i]);
        // Returns 1 if argument is an alphanumeric character.
        // Returns 0 if argument is neither an alphabet nor a digit
        // should add quotes if there are special character(e.g $ | empty space)
        if(!isalnum(str[i])){
            //there are some non-alnum but it will be allowed to show without quotes
            //after test, original ls will also allowed ". or _ or - or + or ~ or % or @ or #'
            //Linux will not allow to create filename with '/', we allow it for display a directory normally
            if(str[i] == '.' || str[i] == '_' || str[i] == '-' || str[i] == '+' || str[i] == '~' || str[i] == '%' || str[i] == '@' || str[i] == '#' || str[i] == '/'){
                continue;
            }else{
                //printf("non alnum! add quotes!  -- %c \n", str[i]);
                return true;  
            } 
        }
    } 
    //puts("not add quotes! \n");
    return false;
}


char* filteredFilenameOrDir(char *fileOrDir){
    //fileOrDir = "123ds";
    if(shouldAddQuotes(fileOrDir)){
        //char newStr[strlen(fileOrDir)+2+1];
        memset(printPaperForPrintQuotes,0, sizeof(char) * PATH_MAX);
        strcpy (printPaperForPrintQuotes,"\'");
        strcat (printPaperForPrintQuotes,fileOrDir);
        strcat (printPaperForPrintQuotes,"\'");
        //printf("%s",printPaperForPrintQuotes);
        return printPaperForPrintQuotes;
    }else{
        //printf("%s",fileOrDir);
        return fileOrDir;
    }
}

char* realPath(char *fileOrDir){ 
    memset(printPaperForFilter,0,sizeof(char) * PATH_MAX);
    if(realpath(fileOrDir, printPaperForFilter) == NULL){
        printf("Realpath Error:can not open the dir = <%s> , program exit now! \n",fileOrDir);
        freeLists();
        exit(EXIT_SUCCESS);
    }else{
        //printf("realfullpath = \n%s\n",printPaperForFilter); 
        return printPaperForFilter;
    } 
}
  
void printPermission(struct stat buf){
    if(S_ISDIR(buf.st_mode)){
        printf("d");
    }
    else if(S_ISLNK(buf.st_mode)){
        printf("l"); //link
    }
    else {
        printf("-");
    }


    if (buf.st_mode & S_IRUSR){
        printf("r");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IWUSR){
        printf("w");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IXUSR){
        printf("x");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IRGRP){
        printf("r");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IWGRP){
        printf("w");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IXGRP){
        printf("x");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IROTH){
        printf("r");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IWOTH){
        printf("w");
    }
    else{
        printf("-");
    }


    if (buf.st_mode & S_IXOTH){
        printf("x");
    }
    else{
        printf("-");
    } 

    printf("    ");
}


void printTime(struct stat buf){
    struct tm *time = localtime(&buf.st_mtime);
    const char * months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    printf("%3s %2d %04d %02d:%02d    ",months[time->tm_mon], time->tm_mday, time->tm_year+1900, time->tm_hour, time->tm_min);
}


void setKeys(bool I, bool L, bool R){
    I_Key = I;
    L_Key = L;
    R_Key = R;
}

void freeLists(){
    List_free(topLevelDirs,releaseItem);
    List_free(topLevelSymbolicDirs,releaseItem);
    List_free(topLevelFiles,releaseItem);
}
 

void printAllLists(){
    printCenter(topLevelFiles);
    if(List_count(topLevelFiles) >0){
        if(L_Key == false){
            puts("\n");
        } 
    }
    printCenter(topLevelSymbolicDirs);
     
    printCenter(topLevelDirs);
}

void printList(List * list, char* listname){
    printf("start to print list : %s \n", listname);
    void *p = List_first(list);
    while(p){
        char *x = (char *)p;
        printf("  %s  ", x);
        p = List_next(list);
    }
    printf("\n=================================================== : \n\n\n\n\n\n");
}


void releaseItem(void * pItem){
    if (pItem){
        free(pItem);
        pItem = NULL;
    }
}

//replace the empty space with '\(space) to avoid wordexp() split before the ' ' which is not allowed!
//https://www.cnblogs.com/uncleyong/p/6898204.html
char* blankEscape(char * originalStr){
    int i, j,  count;
    count = 0;
    for (i=0; i<strlen(originalStr); i++){
        if (originalStr[i] == ' '){
            count++;
        }
    }
    i = strlen(originalStr);
    j = count + strlen(originalStr); 

    if(i == j){
        return originalStr;
    }
    char *arr = malloc(sizeof(char) * (j+1));//strlen() does not include the /0 at the end. must plus 1
    memset(arr, 0,  sizeof(char) *(j+1));
    strcpy(arr, originalStr);
    printf("before：%s\n", originalStr);

    while (i!=j && i>=0) {
        /* if(i<strlen(originalStr) && j < strlen(arr)){
            printf("i = (%d)%c, j = (%d)%c \n",i,originalStr[i], j,arr[j]);
        } */
        
        if (originalStr[i] == ' ') {
            arr[j] = ' ';
            j--;
            arr[j--] = 92; 
            i--;
        }
        else { 
            arr[j] = originalStr[i];  
            j--;
            i--;
        }
    }
    printf("after：%s\n", arr);
    //return ".././a4 (copy)";
    //arr = originalStr;
    return arr;
}

/*  BACKUP (wordexp_t implementation. it was dropped caz it does not support () and some special characters in the filenames)
https://pubs.opengroup.org/onlinepubs/007908799/xsh/wordexp.html
wordexp_t prevent |   &   ;   <   > and () < It also must not contain unquoted parentheses or braces, except in the context of command or variable substitution.>
void generateTopDirs(char* path){
    printf("%s ============= \n", path);
    //decide not to trim the original path if user types it in a form of [ls -i " /home"]
    //Note: there is an empty space in front of directory and users use Double quotes for the directory
    //original linux 'ls' command will not support [ls -i " /home"] nor [ls -i "/home "], so i will not trim the path neither
    
    //1.wordexp() will split at the emtpy space, in order to keep the space, we need to escape by '\ ' 
    // char* newPath = blankEscape(path);

    //decide not to get real full path -- caz assignment requirement needs us to display the original paths
    //https://stackoverflow.com/questions/2341808/how-to-get-the-absolute-path-for-a-given-relative-path-programmatically-in-linux
    //realpath(newPath, fullRealPath);

    //decide not to use 'glob function' for filter handling(for filters, '/home/*as*4/...) caz it will not catch tilde(~)
    //printf("%s  ++ length: %ld \n", newPath, strlen(newPath));
    
    wordexp_t result; //$IFS can not be set here, have to escape empty space
    if (0==wordexp(newPath, &result, 0)){
        char **linkP= result.we_wordv; 
        for(;*linkP;linkP++){ 
            if(canOpenLink(*linkP)){
                //printf("%s  --@@ length: %ld \n", *linkP, strlen(*linkP));
                //move the cursor to the first item
                char *originalPath = malloc(sizeof(char) * PATH_MAX);
                strcpy(originalPath, *linkP);

                //char temp[PATH_MAX];  *** dont do this, it will make weird bug![can only add one item!] 
                //decide not to check duplicate in the list, caz original 'ls' will not filter out the duplicate neither. 
                
                //if the symbolic refer to dir, it will be treated as a folder, else, it will be treated as regular file
                if(isFile(originalPath)){
                    List_last(topLevelFiles);
                    List_append(topLevelFiles, originalPath);  
                }else if(isDir(originalPath)){
                    List_last(topLevelDirs);
                    List_append(topLevelDirs, originalPath);  
                }else{
                    //for debug
                    printf("we will only handle (regular | dir | symbolic link) file! \n");
                    ;
                }
            }else{
                printf("the Link: <%s> is invalid and can not be processed! Program terminated now.. \n", filteredFilenameOrDir(*linkP));
                wordfree(&result); 
                freeLists();
                if(newPath!=path){free(newPath);};
                exit(EXIT_SUCCESS);
            }
        } 
    }
    else{
        printf("Error occurs while parsing the Link: <%s> ! -- It might be invalid! Please check. \nProgram terminated now.. \n",  path );
        wordfree(&result); 
        freeLists();
        if(newPath!=path){free(newPath);};
        exit(EXIT_SUCCESS);
    }
    wordfree(&result); 
    if(newPath!=path){free(newPath);}; 
    //printList();//for debug
    
    
}
 */