#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>



// Function that checks the start of a line with a specific keyword 
int starts_with(char *line, char *keyword){
	while (*line && *keyword){
		if (*line++ !=  *keyword++){
			return 0; // 0 is returned if the line and keyword don't match 
		}
	}
	return *keyword=='\0'; // 1 is returned if he keyword matches the line fully 
}

// Function checks if a specific string is contained in the line 
int contains(char *line, char *c) {
	char *p1,*p2;

	while (*line) {
		p1=line;
		p2=c;
		while  (*p1 && *p2 &&  *p1==*p2){
			p1++;
			p2++;

		}
		if (*p2=='\0'){
			return 1; // match found for substring
		}
		line++;
	}
	return 0; // match not found
}



// function that checks the syntax of the .ml file
int syntax_ml(char *filename){
	char line[256];
	int line_num=1;
	
    // Needs to check if the .ml extension is in the filename 
	if (contains(filename,".ml")){

        // opening file and reading it 
		FILE *fp = fopen(filename,"r");
		if(fp==NULL){
			fprintf(stderr,"Error opening %s or %s does not exit\n",filename,filename);
			exit(EXIT_FAILURE);
		}

        // reading the file line by line and removing the trailing whitespaces from each line 
		while (fgets(line, sizeof(line), fp)){
		;

        
        // Empty lines are ignored 
		if (strlen(line)==0 ){
			line_num++;
			continue;
            }
        
        
        else if(starts_with(line,"\0")){
            line_num++;
            continue;
        }

        // Lines starting with a comment (#) are ignored 
		else if (starts_with(line,"#")){
				line_num++;
				continue;
                }

        // Lines with parenthesis and tabs are ignored
		else if (starts_with(line,"\t") || contains(line,"(")){
			line_num++;
			continue;
		}

		// function syntax is checked 
		else if (starts_with(line, "function")){
			line_num++;
			continue;
		}

        // syntax error if line has no matches with viable patterns 
		else if (!(contains(line,"<-") || contains(line,"print") || starts_with(line,"function") || contains(line,"return") ) && strlen(line)>0){
			fprintf(stderr,"!Syntax error on line %d \n",line_num);
			fclose(fp);
                        exit(EXIT_FAILURE);
			}
		else{
			line_num++;
			continue;
        }
		
		line_num++;
		}

        // file is closed
		fclose(fp);
		return 0;
	}

	else {
		fprintf(stderr,"!Invalid FIle Type : .ml file required \n");
		exit(EXIT_FAILURE);
	}
	
	}

// Function converts the .ml code into c code 
void c_conversion(char * filename){
	char line[256];
	int line_num=1; // line number counter is set to 1- tracking the current line being processed 
	int in_func=0; // Checks if inside a function
	int aft_ret=0; // checks if it after a return statement 
	int in_main=0; // checks if inside the main function 
	int var = 0; // checks if there are declared variables 	
	
	// output filename created after getting the current process ID 
	int pid=getpid();
	char fil[50];

	snprintf(fil, sizeof(fil), "ml-%d.c", pid);

    // output file opened for writing 
	FILE *file= fopen(fil,"w");
	if (file ==NULL){
		fprintf(stderr, "Error creating output file\n");
		exit(EXIT_FAILURE);
	}
	
    // input file opened for reading
	FILE *fp= fopen(filename,"r");
	if (fp== NULL){
		fprintf(stderr,"Error opening file\n");
		exit(EXIT_FAILURE);
	}

    // Standard C headers are written in the output file 
	fprintf(file, "#include <stdio.h>\n");
	fprintf(file, "#include <stdlib.h>\n");
	fprintf(file ,"#include <unistd.h>\n");

    // input .ml file read line by line 
	while (fgets(line,sizeof(line),fp)) {
		if (starts_with(line,"#")){
			continue;
		}

        // Empty lines are handled + Functions are closed 
		else if (strlen(line)==0){
			line_num++;

			if (in_func==1){
				fprintf(file, "} \n"); // Function is terminated in c code
				in_func=0;
			}
			continue;
		}
        
        // variable declaration is handled with '<-'
		else if(contains(line,"<-")){
			char *ident=line;
			char *value=strchr(line,'<');
			*value='\0';
			value+=2;

            
			
			fprintf(file, " double %s = %s;\n",ident,value);
			var=1;
			
		}

        // Function declarations are handled
		else if (contains(line,"function ") && !aft_ret && !in_main){
		
    	    char *func=line + 9;
			char *token;
			char params[100]; // empty parameter list is initialised

            // getting the function name 
			token = strtok(func," ");  
			char *func_name =token;

            // function parameters are extracted
			while ((token = strtok(NULL," ")) !=NULL){
				strcat(params, "double ");
				strcat(params, token);
				strcat(params,",");
			}
            // commas from parameters are removed 
			if (params[strlen(params) -1] ==','){
				params[strlen(params)-1] = '\0';
			}
			fprintf(file , "double %s(%s) { \n", func_name, params); // Writing the C function header
			in_func = 1;

			in_func=1;
		// handling print statements and variable assignments (inside the function)
		while(in_func==1 && fgets(line,sizeof(line),fp)){
		if (starts_with(line,"#")){
			line_num++;
			continue;
		}
		else if (strlen(line)==0){
			line_num++;
			
			continue;
		}

		else if(contains(line,"<-") && starts_with(line,"	")){
			
			char *ident=line;
			char *value=strchr(line,'<');
			*value='\0';
			value+=2;
			fprintf(file, " double %s = %s;\n",ident,value);
		}
		
        // Handling print statements
		else if (contains(line,"print ") && starts_with(line,"	")){
			char *stat=line+6;

            // checking if there is a string in the print function		
			if (stat[0]=='"' && stat[strlen(stat)-1]=='"'){
				fprintf(file,"printf(%s\\n);\n",stat);
				
			}
			
            // handling prints of floats and integers 
			else if (contains(stat,".")){
					fprintf(file, "printf(\"%%.6f\\n\", %s);\n", stat); 
				}

			else if( isdigit(stat[0]) && !contains(stat,".")){
					fprintf(file, "printf(\"%%d\\n\", %s);\n", stat); 	
				}
		
            // Handling mixed types 
			else {
				fprintf(file, "if (%s == (int)%s) {\n", stat, stat);  
        		fprintf(file, "    printf(\"%%d\\n\", (int)(%s));\n", stat);  
        		fprintf(file, "} else {\n");
        		fprintf(file, "    printf(\"%%lf\\n\", (double)(%s));\n", stat);  
        		fprintf(file, "}\n"); 
		}
		
	    aft_ret=1;
	}
		
        // Return statments are handled 
		else if (contains(line,"return ") && starts_with(line,"	")){
			char *ret_stat=line+7;
			
			fprintf(file,"return %s; \n",ret_stat);
			aft_ret=1;
	} 
		
        // function is closed after return
		else if (in_func==1 && aft_ret==1 && (!starts_with(line,"	") || starts_with(line,"#"))){
			
			fprintf(file,"}\n");
			aft_ret=0;
			in_func=0;
		}
	}
}
    
        // checking if main function is needed to start
		if ((starts_with(line,"print")  || starts_with(line,"return") || contains(line,"<-") || starts_with(line,"#") ) && !in_main){
			fprintf(file, "int main(int argc, char *argv[]){\n");
			in_main=1;	
		}
	
        // Handling print statements + other statments inside the main function
		if (in_main){

            // lines starting with # skipped 
		    if (starts_with(line,"#")){
			    line_num++;
			    continue;
		    }
		
        // Lines starting with 'print' are handled 
		else if (starts_with(line,"print ")){
			char *stat=line+6;
			
            // if statement is a string
			if (stat[0]=='"' && stat[strlen(stat)-1]=='"'){
				fprintf(file,"printf(%s\\n);\n",stat);
			}

            // if statement has a decimal point
			else if (contains(stat,".")){
					fprintf(file, "printf(\"%%.6f\\n\", %s);\n", stat); 
				}

            // If statement is a digit and has no decimal point 
			else if( isdigit(stat[0]) && !contains(stat,".")){
					fprintf(file, "printf(\"%%d\\n\", %s);\n", stat); 	
				}

            // Check the type of statement contains a variable 
			else {
                // checking the variable can be an int 
				fprintf(file, "if (%s == (int)%s) {\n", stat, stat);  
        		fprintf(file, "    printf(\"%%d\\n\", (int)(%s));\n", stat);  
        		fprintf(file, "} else {\n");
        		fprintf(file, "    printf(\"%%lf\\n\", (double)(%s));\n", stat);  
        		fprintf(file, "}\n"); 
		}	
	}

        // Un indented lines are handled
		else if (!starts_with(line,"	") && strlen(line)>0 && !starts_with(line,"#") && in_func==0 && !contains(line,"function ")){
			fprintf(file,"%s;",line);
			
		}
	}
}

    // Checking if not in main function yet and start of main function is encountered
	if (var==1 && in_main==0 && in_func==0 && aft_ret==0){
		fprintf(file, "int main(int argc, char *argv[]){\n");

		in_main=1;// Flag that we are inside main function
		var=0;
	}

    // main functions closed if in it 
	if (in_main==1){
	    fprintf(file,"return 0;\n"); // return statement added for main function
	    fprintf(file,"}\n"); // main function closed 	
	    in_main=0; // Flah that we aren't in the main function anymore 
	}

    // Input and Output files closed 
	fclose(fp);
	fclose(file);
}

// File input and conversion, handled by the main function 
int main(int argc, char *argv[]){
	if (argc<2){
		fprintf(stderr,"Usage: %s filename\n",argv[0]); // error if no file provided
		exit(EXIT_FAILURE); // thus exit when no file provided
		}
	else {
		// checking ml syntax of file
		if (syntax_ml(argv[1])==0){
			c_conversion(argv[1]); //ml to c conversion occurs if syntax is ok

			int filen=getpid(); // creating new filename using process ID
			char filna[50];

			snprintf(filna, sizeof(filna),"ml-%d.c",filen);

            // Command for compiling generated c file prepared
			char command[256];

			snprintf(command, sizeof(command), "cc -o translate %s",filna);

			// Compilation executed 
			if (system(command) != 0) {
				    fprintf(stderr, "Compilation failed\n");
    				    exit(EXIT_FAILURE);
			}		

            // run successfully compiled program 
			if (system("./translate")==0){
                // ccompiled binary deleted if execution is ok
				system("rm ./translate");

				char exec[256]; // generated c file deleted
				sprintf(exec,"rm %s",filna);
				system(exec);
			}
		// succesfilly exiting after doing everything
		exit(EXIT_SUCCESS);
		}
	}

}
