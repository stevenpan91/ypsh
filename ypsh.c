//Some concepts were taken from "lsh" by Stephen Brennan
//at brennan.io/2015/01/16/write-a-shell-in-c
//Particularly the main function and loop function
//The rest was coded by myelf

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<dirent.h>
#include<time.h>
#include<pwd.h>
#include<grp.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

#define MAX_STR_SIZE 500
#define STD_STR_BUFFER 50
#define PRNT_BUFFER 50

#define prbfub printbuffer[uboundprntbuf]

typedef int bool;
#define true 1
#define false 0

int argamounts;
int unexecutedargsub;
int uboundprntbuf;
char *thedirectory;
char *redirfilepath;
char **printbuffer;

bool redirectflag;
bool appendflag;

//Functions shell can use
int shexit(char** args);
int wfile(char** args);
int change_directory(char** args);
int getdirectory(char** args);
int ypsh_echo(char** args);
int ypsh_ls(char** args);

char *funcnames[]={
    "shexit",
    "wfile",
    "cd",
    "getdir",
    "echo",
    "ls"
};

int (*funcs[])(char **) = {
    &shexit,
    &wfile,
    &change_directory,
    &getdirectory,
    &ypsh_echo,
    &ypsh_ls
};

int funcnum(){
    return sizeof(funcnames)/sizeof(char*);
}

void ypsh_loop(void);
char * read_line(void);
char ** read_args(char* line);

//get string length
int slen(char *thestring)
{
    int charcount=0;
    while (*thestring !='\0')
    {
        charcount++;
        thestring++;

    }

    return charcount;

}

//check if string equal
bool sequal(char* str1, char* str2){
    if(slen(str1)!=slen(str2))
        return false;

    for(int i=0;i<slen(str1);i++)
    {
        if(str1[i]!=str2[i])
            return false;
    }
    return true;
}

char **parsecmd(char *fullline)
{
    //array of possible delimiters
    int dlamount=3;
    char* delimiters=malloc(sizeof(char)*3);
    delimiters[0]=' ';
    delimiters[1]='|';
    delimiters[2]='>';

    //for use in a for loop to run through delimiters
    int checkdl;

    //create an array of strings and allocate memory for all strings within array
    char** returnarray=malloc(100*sizeof(char*));
    int initdblptr;
    for (initdblptr=0;initdblptr<100;initdblptr++)
        returnarray[initdblptr]=calloc(STD_STR_BUFFER,sizeof(char));

    //Subscript of strings, at -1 when none
    int rtnarrayindx=-1;

    //for copying to arg
    char *tempstr=calloc(STD_STR_BUFFER,sizeof(char));
    //char tempstr[STD_STR_BUFFER];
    int tempstrindx=0;

    //hold delimiter to copy into arguments
    char *tempdelim=calloc(3,sizeof(char));

    //copy line read from shell
    char strbuffer[STD_STR_BUFFER];
    int strbufferindx=0;
    
    //copy the inputted line to manipulate it
    strcpy(strbuffer,fullline);

    int lastcharpos;
    lastcharpos=0;

    int charpos;
    charpos=0;

    //for use in for loop
    int chopstring;

    bool divisionflag;
    bool doubledelim; //interpret if delim is >>
    doubledelim=false;

    while(strbuffer[strbufferindx] != '\0')
    {
    divisionflag=false;
        for(checkdl=0;checkdl<dlamount;checkdl++)
        {
            if(strbuffer[charpos]==delimiters[checkdl] && delimiters[checkdl]!=' ')
            {
                divisionflag=true;
		
		//This is mainly to account for append command >>
		if(strbuffer[charpos-1]==delimiters[checkdl]){
		doubledelim=true;
		
		//add the delimiter twice into tempdelim	
		tempdelim[0]=delimiters[checkdl];
		tempdelim[1]=delimiters[checkdl];
		 		
		}
		else{
		//add delimiter once
                tempdelim[0]=delimiters[checkdl];
		}
            }
        }

        //split at delimiter if divisionflag is true
        if(divisionflag==true && doubledelim==false){
            divisionflag=false;
            
	    //increase return array index to add new argument and increase argamounts(global var)
	    rtnarrayindx++;
            argamounts++;
	    
            //copy from last character position of input line up to this division flag character
            for (chopstring=lastcharpos;chopstring<charpos;chopstring++)
            {
                tempstr[tempstrindx]=strbuffer[chopstring];
                tempstrindx++;
            }
	    //add null char to end
            tempstr[tempstrindx]='\0';

            //move lastcharpos to current charpos but skip the delimiiter (hence charpos+1) as long
            //as charpos isn't the last character already for some reason
            if(charpos<slen(strbuffer)-1){
		lastcharpos=charpos+1;
	    }
            else{
            lastcharpos=slen(strbuffer)-1;}

            tempstrindx=0;

            strcpy(returnarray[rtnarrayindx],tempstr);

            //add the delimiter itself to the next argument
            rtnarrayindx++;
            strcpy(returnarray[rtnarrayindx],tempdelim);
            argamounts++;



        }
		//if it's a double delim skip next delim too
		else if(doubledelim==true)
		{
		//skip this charpos
		lastcharpos=charpos+1;
		doubledelim=false;
		strcpy(returnarray[rtnarrayindx],tempdelim);
		}

        charpos++;
        strbufferindx++;
    }

    //at end need to take the remaining strings and parse into returnargs
        rtnarrayindx++;
        argamounts++;

        for (chopstring=lastcharpos;chopstring<slen(strbuffer);chopstring++)
        {
            tempstr[tempstrindx]=strbuffer[chopstring];
            tempstrindx++;
        }
        tempstr[tempstrindx]='\0';
        tempstrindx=0;


        strcpy(returnarray[rtnarrayindx],tempstr);



    free(tempdelim);
    free(tempstr);
    free(delimiters);

    return returnarray;
}

char* dlinterp(char *line)
{
    int dlamount=3;
    char* delimiters=malloc(sizeof(char)*3);
    delimiters[0]=' ';
    delimiters[1]='|';
    delimiters[2]='>';

    //used in for loop to check delimiters
    int checkdl;

    char* returnline=malloc(sizeof(char)*STD_STR_BUFFER*4);
    int charpos;
    charpos=0;

    //check characters around whitespaces
    char* nextchar;
    char* prevchar;
    bool skip=false;

    while(*line != '\0')
    {
        //set skip chars as false first
        skip=false;

        //for (checkdl=0; checkdl<dlamount; checkdl++)
        //    {
                //if(*line==delimiters[checkdl])
                if(*line==' ')
                {
                    //read next char
                    nextchar=line+1;
                    prevchar=line-1;
                    //cycle through all delimiters
                    for (checkdl=0; checkdl<dlamount; checkdl++)
                    {

                        //if nextchar is not null
                        if (*nextchar!='\0' && *prevchar!='\0')
                        {
                            //if the next char is a delimiter then skip this space
                            if(*nextchar==delimiters[checkdl] || *prevchar==delimiters[checkdl]){
                            skip=true;

                            }
                        }
                    }


                }

          //  }

            //Only add if skip is false
            if(skip==false){
                returnline[charpos]=*line;
                charpos++;
            }

        //Move to next char in line
        line++;
    }

    returnline[charpos]='\0';

    free(delimiters);
    return returnline;


}



//Readline, readargs, execute sections
char* read_line(void)
{
	char* inputline;
	//allow up to 100 chars in a line for now
	inputline=malloc(sizeof(char)*100);
	//scanf("%99[^\n]",inputline);

    //get input from user
	fgets(inputline,MAX_STR_SIZE,stdin);

    //switch out line return with null at end of string
	if ((strlen(inputline)>0) && (inputline[strlen (inputline) - 1] == '\n'))
        inputline[strlen (inputline) - 1] = '\0';

	return inputline;

}

char** read_args(char* line)
{
    argamounts=0;
    //tokens and strtok was taken from tutorialspoint regarding the strtok function
    char** returnargs;

    char* dlcheckline;
    if(line[0]!='\0'){
        //printf("The line is %s.\n",line);
        //below returns line without spaces between words and delimiters
        //For instance 'ls | grep' would be 'ls|grep'. This is a temporary measure
        dlcheckline=dlinterp(line);}
    else{
        //printf("Line is null.");
        dlcheckline=line;
    }

    returnargs=parsecmd(dlcheckline);
    free(dlcheckline);

    return returnargs;
}


//For now only execute functions that are found, not priority
int shexecute(char** args)
{

    //array of possible delimiters
    int dlamount=4;
    char** delimiters=malloc(sizeof(char*)*4);
    delimiters[0]=" ";
    delimiters[1]="|";
    delimiters[2]=">";
    delimiters[3]=">>";
	
    int initsepargs;
    char** separgs=calloc(10,sizeof(char*));
    for(initsepargs=0; initsepargs<10;initsepargs++)
	separgs[initsepargs]=calloc(STD_STR_BUFFER,sizeof(char));

    //printf("%s\n", args[0]);
    //default is keep loop
    int finalreturn=1;
    int checkfunc;
    int runargs;
    //bool argneverfound;
    //argneverfound=false;

    bool argfound;
    argfound=false;

//    //to check whether the arg before is a pipe to pass argument. Only for non-first command
//    bool haspipe;
//    haspipe=false;

    //check if arg is delimiter
    bool isdelim;
    isdelim=false;
    char* thedelim=malloc(sizeof(char)*3);
    int checkdl;

    char strbackup[40];

    for (runargs=0; runargs<argamounts; runargs++){

        for(checkdl=0;checkdl<dlamount;checkdl++)
        {
            if(strcmp(args[runargs],delimiters[checkdl])==0 )
            {
                isdelim=true;
                strcpy(thedelim,args[runargs]);
            }
        }

        if(isdelim){
	    isdelim=false;
            if(strcmp(thedelim,">")==0){
                redirectflag=true;
		printf("runargs+1 is %s\n",args[runargs+1]);
                strcpy(redirfilepath,args[runargs+1]);

            }
	    else if(strcmp(thedelim,">>")==0){
		appendflag=true;
		strcpy(redirfilepath,args[runargs+1]);
		printf("runargs+1 is %s\n",args[runargs+1]);

            }
	    /*else if(strcmp(thedelim,"|")==0){
		int loopsepargs;
		for(loopsepargs=0;loopsepargs<runargs;loopsepargs)
			strcpy(separgs[loopsepargs],args[loopsepargs]);	    

	    }*/

        }


        else if(redirectflag==false && appendflag==false){
            //reset argfound
            argfound=false;
                for (checkfunc=0; checkfunc<funcnum(); checkfunc++)
                {
                    //Copy whole line and use strtok to check only the first part of string to see if it's a valid command

                    strcpy(strbackup,args[runargs]);




                    if (strcmp(funcnames[checkfunc],strtok(strbackup," "))==0)
                    {
			strcpy(separgs[0],args[runargs]);
                        //flag that an arg is found
                        argfound=true;

                        //Execute all commands before final one
                        if (runargs<argamounts-1){
                        //printf("%s\n", "non-last arg exec was reached");
                        (*funcs[checkfunc])(separgs);}
                        //final command gives status
                        else if (runargs==argamounts-1 && args[runargs]!=NULL){
                        finalreturn=(*funcs[checkfunc])(separgs);}
                        else{
                        printf("Error reading multiple arguments");
                        finalreturn=0;}

                    }
                    //free(strbackup);
                }//cycle through functions for loop

                //if there wasn't an argument found output that command is not found
                if(argfound==false){
                printf("No such command %s.\n",strbackup);
                finalreturn=1;
                }
            }//non delim cycle

    }//runargs for loop
    free(thedelim);
    free(delimiters);

   
    for(initsepargs=0; initsepargs<10;initsepargs++)
	free(separgs[initsepargs]);

    free(separgs);

    return finalreturn;

}


// ** Shell functions start here **

int shexit(char** args){
return 0;
}

int ypsh_ls(char **args){
    DIR *thisdir;
    struct dirent *thefile;
    struct stat thestat;

    char* tempstr=malloc(sizeof(char)*STD_STR_BUFFER*4);
    char* tempstr2=malloc(sizeof(char)*STD_STR_BUFFER);
    char date[16];
    int formatspace=15;
    int startspace;

    thisdir=opendir(thedirectory);
    while((thefile=readdir(thisdir)) != NULL)
        {
        stat(thefile->d_name,&thestat);
        //sprintf(tempstr,"%10s ", thestat.st_mode); 
        strcpy(tempstr, (S_ISDIR(thestat.st_mode)) ? "d" : "-");
            strcat(tempstr, (thestat.st_mode & S_IRUSR) ? "r" : "-");
            strcat(tempstr, (thestat.st_mode & S_IWUSR) ? "w" : "-");
            strcat(tempstr, (thestat.st_mode & S_IXUSR) ? "x" : "-");
            strcat(tempstr, (thestat.st_mode & S_IRGRP) ? "r" : "-");
            strcat(tempstr, (thestat.st_mode & S_IWGRP) ? "w" : "-");
            strcat(tempstr, (thestat.st_mode & S_IXGRP) ? "x" : "-");
            strcat(tempstr, (thestat.st_mode & S_IROTH) ? "r" : "-");
            strcat(tempstr, (thestat.st_mode & S_IWOTH) ? "w" : "-");
            strcat(tempstr, (thestat.st_mode & S_IXOTH) ? "x" : "-");
        
    /*	printf( (S_ISDIR(thestat.st_mode)) ? "d" : "-");
            printf( (thestat.st_mode & S_IRUSR) ? "r" : "-");
            printf( (thestat.st_mode & S_IWUSR) ? "w" : "-");
            printf( (thestat.st_mode & S_IXUSR) ? "x" : "-");
            printf( (thestat.st_mode & S_IRGRP) ? "r" : "-");
            printf( (thestat.st_mode & S_IWGRP) ? "w" : "-");
            printf( (thestat.st_mode & S_IXGRP) ? "x" : "-");
            printf( (thestat.st_mode & S_IROTH) ? "r" : "-");
            printf( (thestat.st_mode & S_IWOTH) ? "w" : "-");
            printf( (thestat.st_mode & S_IXOTH) ? "x" : "-");
    */
        sprintf(tempstr2,"%3d ", thestat.st_nlink);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);
        
        
        sprintf(tempstr2,"%20s", getpwuid(thestat.st_uid)->pw_name);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);

        sprintf(tempstr2, "%15s", getgrgid(thestat.st_gid)->gr_name);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);
        

        /*sprintf(tempstr2,"%10d ",thestat.st_uid);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);

        sprintf(tempstr2,"%10d ",thestat.st_gid);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);
            */

        sprintf(tempstr2,"%10d  ",thestat.st_size);	
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);

        strftime(date,16,"%d-%b-%y %H:%M",localtime(&(thestat.st_mtime)));
        sprintf(tempstr2,"%16s ",date);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);
        /*for(startspace=0;startspace<=formatspace-slen(tempstr);startspace++)
            {
            memcpy(tempstr+slen(tempstr)," ",slen(" ")+1);
        
            }*/
        sprintf(tempstr2, "%20s",thefile->d_name);
        memcpy(tempstr+slen(tempstr),tempstr2,slen(tempstr2)+1);
        
        uboundprntbuf++;
        strcpy(prbfub,tempstr);
        
        
        }

    closedir(thisdir);
    free(tempstr);
    free(tempstr2);

    return 1;
}


int ypsh_echo(char** args){

    char *strbuffer=calloc(STD_STR_BUFFER,sizeof(char));

    int checkargs;

    strcpy(strbuffer,args[0]);
    uboundprntbuf++;
    strcpy(prbfub,strbuffer+5);
    free(strbuffer);

    return 1;
}

int getdirectory(char** args){
    //printf("getdir was reached.\n");
    char* diroutput=malloc(sizeof(char)*STD_STR_BUFFER);
    getcwd(diroutput,STD_STR_BUFFER);
    //printf("The current working directory is: %s \n",diroutput);

    uboundprntbuf++;
    strcpy(prbfub,"The current working directory is: ");
    memcpy(prbfub+slen(prbfub),diroutput,slen(diroutput)+1);

    free(diroutput);
    return 1;
}


int change_directory(char** args){
    char* firstline;
    firstline=args[0];

    char strbackup[STD_STR_BUFFER];
	
    //don't copy the command cd
    strcpy(strbackup,firstline+3);
    if(sequal("..",firstline+3)){

    }
    else{
        //if(chdir(strbackup)==0){
        DIR* dir=opendir(strbackup);
        if(dir){
            strcpy(thedirectory,strbackup);
        }
        else{
            perror("Invalid directory");
        }
    }

    return 1;
}




//-------------------------------------------------------------------------------------------
//wfile section
int encrypt(void);
int decrypt(void);
int normal(void);

FILE *fp1, *fp2;
char ch;

int wfile(char ** args)
{
    bool wfileloop = true;
    int choice;
    while(wfileloop)
    {
        printf("Select One of the Following:\n");
        printf("\n1. Encrypt\n");
        printf("2. Decrypt\n");
        printf("3. Normal\n");
        printf("4. Exit\n");
        printf("Enter here: \t");
        scanf("%d%*c", &choice);
        switch(choice)
        {
            case 1: encrypt(); break;
            case 2: decrypt(); break;
            case 3: normal(); break;
            case 4: wfileloop=false; return 1;//ypsh_loop();
        }

    }
    printf("\n");
    return 0;
}



int normal()
{
    char a[] = "\nThis is Normal Mode\n";
    printf("%s%s", a, "File is not encrypted.\n");

    int num;

    char *filepath;
    filepath=  malloc(100*sizeof(char));
    char *linetoFile;
    linetoFile=  malloc(30*sizeof(char));
    char *printmode;
    printmode =malloc(5*sizeof(char));
    printf("Please enter the filepath with extension: ");
    fgets(filepath,MAX_STR_SIZE,stdin);

    if ((strlen(filepath)>0) && (filepath[strlen (filepath) - 1] == '\n'))
        filepath[strlen (filepath) - 1] = '\0';


    printf("Enter print mode (a to append, w to rewrite): ");
    fgets(printmode,MAX_STR_SIZE,stdin);

    if ((strlen(printmode)>0) && (printmode[strlen (printmode) - 1] == '\n'))
        printmode[strlen (printmode) - 1] = '\0';


    fp1 = fopen(filepath,printmode);

    if (fp1==NULL)
    {
        printf("Error!");
        exit(1);
    }

    printf("Enter type of input to file (s for string, i for integer): ");

    char *inputType;
    inputType=malloc(2*sizeof(char));


    fgets(inputType,MAX_STR_SIZE,stdin);
    if ((strlen(inputType)>0) && (inputType[strlen (inputType) - 1] == '\n'))
        inputType[strlen (inputType) - 1] = '\0';

    if (strcmp(inputType,"i")==0){
    printf("Enter integer(s): ");
    scanf("%d", &num);
    fprintf(fp1, "%d\n",num);

    }
    else if (strcmp(inputType,"s")==0){
    printf("Enter string: ");

    fgets(linetoFile,MAX_STR_SIZE,stdin);
    if ((strlen(linetoFile)>0) && (linetoFile[strlen (linetoFile) - 1] == '\n'))
        linetoFile[strlen (linetoFile) - 1] = '\0';

    fprintf(fp1, "%s\n",linetoFile);
    }





    fclose(fp1); //close pointer

    free(filepath);
    free(linetoFile);
    free(printmode);
    free(inputType);
    return 0;
}

int encrypt()
{
    char a[] = "\nThis is Encryption Mode\n";
    printf("%s%s", a, "File is encrypted.\n");

    int num;

    char *filepath;
    filepath=  malloc(100*sizeof(char));
    char *linetoFile;
    linetoFile=  malloc(30*sizeof(char));
    char *printmode;
    printmode = malloc(5*sizeof(char));
    printf("Please enter the filepath with extension: ");
    fgets(filepath,MAX_STR_SIZE,stdin);

    char *inputType;
        inputType=malloc(2*sizeof(char));

    if ((strlen(filepath)>0) && (filepath[strlen (filepath) - 1] == '\n'))
        filepath[strlen (filepath) - 1] = '\0';

    char *eOrw;
    eOrw=malloc(5*sizeof(char));
    printf("Encrypt contents or write/append then encrypt? (e or w): ");
	fgets(eOrw, MAX_STR_SIZE,stdin);
    if ((strlen(eOrw)>0) && (eOrw[strlen (eOrw) - 1] == '\n'))
        eOrw[strlen (eOrw) - 1] = '\0';


    if (strcmp(eOrw,"w")==0)
    {
        printf("Enter print mode (a to append, w to rewrite): ");
        fgets(printmode,MAX_STR_SIZE,stdin);

        if ((strlen(printmode)>0) && (printmode[strlen (printmode) - 1] == '\n'))
            printmode[strlen (printmode) - 1] = '\0';

        fp1 = fopen(filepath,printmode);

        if (fp1==NULL)
        {
            printf("Error!");
            exit(1);
        }

        printf("Enter type of input to file (s for string, i for integer): ");




        fgets(inputType,MAX_STR_SIZE,stdin);
        if ((strlen(inputType)>0) && (inputType[strlen (inputType) - 1] == '\n'))
            inputType[strlen (inputType) - 1] = '\0';

        if (strcmp(inputType,"i")==0){
        printf("Enter integer(s): ");
        scanf("%d", &num);
        fprintf(fp1, "%d\n",num);

        }
        else if (strcmp(inputType,"s")==0){
        printf("Enter string: ");

        fgets(linetoFile,MAX_STR_SIZE,stdin);
        if ((strlen(linetoFile)>0) && (linetoFile[strlen (linetoFile) - 1] == '\n'))
            linetoFile[strlen (linetoFile) - 1] = '\0';

        fprintf(fp1, "%s\n",linetoFile);
        fclose(fp1);
        }

    }//if write option was taken


    fp1 = fopen(filepath,"r+");


    bool keepEncrypt = true;

    while(keepEncrypt)
    {
        ch = fgetc(fp1);
        if(ch == EOF)
        {
            printf("\nEnd Of File\n");
            keepEncrypt=false;
        }
        else
        {
            fseek(fp1, ftell(fp1)-1,SEEK_SET);
            ch = ch - (8 * 5 - 3);
            fputc(ch, fp1);

        }
    }



    fclose(fp1); //close pointer

    free(eOrw);
    free(filepath);
    free(linetoFile);
    free(printmode);
    free(inputType);
    return 0;
}

int decrypt()
{
    char a[] = "\nThis is Decryption Mode\n";
    printf("%s%s", a, "File will be decrypted.\n");


    char *filepath;
    filepath=  malloc(100*sizeof(char));

    printf("Please enter the filepath with extension: ");
    fgets(filepath,MAX_STR_SIZE,stdin);

    if ((strlen(filepath)>0) && (filepath[strlen (filepath) - 1] == '\n'))
        filepath[strlen (filepath) - 1] = '\0';

    char *confirm;
    confirm=malloc(5*sizeof(char));
    printf("Decrypt content? (y or n): ");
	fgets(confirm, MAX_STR_SIZE,stdin);
    if ((strlen(confirm)>0) && (confirm[strlen (confirm) - 1] == '\n'))
        confirm[strlen (confirm) - 1] = '\0';


    if (strcmp(confirm,"y")==0)
    {
        fp1 = fopen(filepath,"r+");

        bool keepEncrypt = true;

    while(keepEncrypt)
    {
        ch = fgetc(fp1);
        if(ch == EOF)
        {
            printf("\nEnd Of File\n");
            keepEncrypt=false;
        }
        else
        {
            fseek(fp1, ftell(fp1)-1,SEEK_SET);
            ch = ch + (8 * 5 - 3);
            fputc(ch, fp1);
        }
    }


    }//if decrypt








    fclose(fp1); //close pointer

    free(confirm);
    free(filepath);

    return 0;
}

//---------------------------------------------------------------------------------------------
//End of wfile section

//loop and main
void ypsh_loop(void)
{
	char *line;
	char **args;
	int status;
    //unexecutedargsub=0;    

    //allocate memory for directory
    thedirectory=calloc(STD_STR_BUFFER,sizeof(char));

    redirfilepath=malloc(sizeof(char)*STD_STR_BUFFER);

    //allocate memory for printing buffer
    int initprntbuf;
    printbuffer=calloc(PRNT_BUFFER,sizeof(char*));
    for(initprntbuf=0;initprntbuf<PRNT_BUFFER;initprntbuf++)
        printbuffer[initprntbuf]=calloc(STD_STR_BUFFER*4,sizeof(char));

    //to print out print buffer
    int printoutindx;
    uboundprntbuf=-1;

    //redirect and append set to false
    redirectflag=false;
    appendflag=false;

    //print out dir alongside shell
    getcwd(thedirectory,STD_STR_BUFFER);




	do {
        printf("ypsh %s> ",thedirectory);
        line = read_line();
        args=read_args(line);

        if (argamounts>0){
            status=shexecute(args);}
        else{
            status=1;}

        //print out the print buffer after execution if redirect and append flags are false
        if (uboundprntbuf>=0 ){
            if(redirectflag==false && appendflag==false){
                for(printoutindx=0;printoutindx<=uboundprntbuf;printoutindx++)
                    {printf("%s\n",printbuffer[printoutindx]);
                    //	printf("test loc of print\n");
                    }
            }
            else if(redirectflag==true){
                redirectflag=false;
                fp1 = fopen(redirfilepath,"w");
                printf("write type redirfilepath is %s",redirfilepath);

                for(printoutindx=0;printoutindx<=uboundprntbuf;printoutindx++)
                    fprintf(fp1,"%s\n",printbuffer[printoutindx]);
                    //fprintf(fp1,"%s\n",printbuffer[0]);
        

                fclose(fp1); //close pointer
            }
            else if(appendflag==true){
                appendflag=false;
                fp1 = fopen(redirfilepath,"a");
                printf("append type redirfilepath is %s\n",redirfilepath);
                printf("uboundprntbuf is %d\n",uboundprntbuf);
                printf("printbuffer[0] is %s\n",printbuffer[0]);
                printf("printbuffer[1] is %s\n", printbuffer[1]);

                    for(printoutindx=0;printoutindx<=uboundprntbuf;printoutindx++)
                        fprintf(fp1,"%s\n",printbuffer[printoutindx]);
                        //fprintf(fp1,"%s\n",printbuffer[0]);
            

                    fclose(fp1); //close pointer
            
            }
            //free print buffer
            int freeprbuf;

            //printbuffer=calloc(PRNT_BUFFER,sizeof(char*));
            for(freeprbuf=0; freeprbuf<PRNT_BUFFER;freeprbuf++){
                   memset(printbuffer[freeprbuf],0,STD_STR_BUFFER*4);
            }

            uboundprntbuf=-1;
            //memset(printbuffer,0,PRNT_BUFFER);

        }

        free(line);
        printf("amount of args %i\n",argamounts);
        int freedouble;
        for(freedouble=0; freedouble<argamounts; freedouble++)
        {
            printf("argument %d is %s address is %s pointer val is %d \n",
            freedouble,args[freedouble],&*args[freedouble],*args[freedouble]);
            //free(args[freedouble]);
        }

        for(freedouble=0; freedouble<100; freedouble++)
        {
            free(args[freedouble]);
        }

        //if(argamounts>0){
        //free(*args);}


        free(args);
        printf("status is %d \n", status);

	}while(status);

        //free print buffer
        int freeprbuf;
        for(freeprbuf=0; freeprbuf<PRNT_BUFFER;freeprbuf++)
                free(printbuffer[freeprbuf]);

        free(printbuffer);
    	
    free(redirfilepath);
    free(thedirectory);
}

int main(int argc, char ** argv)
{
//Load configs

//Run command loop

ypsh_loop();


return EXIT_SUCCESS;
}


