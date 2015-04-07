/*
Copyright 2015 Joao Rietra

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define OPEN_LEN (30)
#define AUX_LEN (30)
#define LINE_LEN (1000)

#define SWITCH(ptr) 	if(true){char* _ptr = ptr;if(false){ 
#define CASE(str)		} else if(!strcmp(_ptr, str)) {
#define DEFAULT()  		} else { 
#define ENDSWITCH() 	}}

#define ERROR() printf("error: line %d\n",lineNumber);exit(0);

typedef enum{false, true} bool;

typedef enum {
	HASH_END = -1,
	HASH_NONE, 
	HASH_IF, HASH_ELSEIF, HASH_ELSE,
	HASH_DOWHILE, HASH_WHILE, HASH_LOOP,
	HASH_FUNCTION
} Hashtag;
Hashtag open[OPEN_LEN];


int lineNumber 	= 0,
	countIF 	= 0,
	curIF		= 0,
	stackIF[AUX_LEN],
	subcountIF[AUX_LEN],
	curLOOP 	= 0,
	countLOOP 	= 0,
	stackLOOP[AUX_LEN],
	funcPushCount;

char line[LINE_LEN], temp[20], func[50], tabs[20];
char v1[50], v2[50], op[10];

FILE* in = NULL;
FILE* out = NULL;

void closeHash();
void openHash(Hashtag);
bool printCond();
int countIFs();
void popIF();
void pushIF();
void popLOOP();
void pushLOOP();


void closeHash(){
	int i, c;
	for(i=OPEN_LEN-1; i>=0; i--){
		if(open[i] != HASH_NONE){
			break;
		}
	}
	switch(open[i]){
		case HASH_IF:
		case HASH_ELSEIF:
			c = countIFs();
			fprintf(out, "%s_if_%d_%d:\n", 
				tabs, curIF, ++subcountIF[c-1]);
		case HASH_ELSE:
			fprintf(out, "%s_if_%d_out:\n", 
				tabs, curIF);
			popIF();
			while(i && open[i] > 1 && open[i] >= open[i-1]){
				open[i] = 0;
				i--;
			}
			open[i] = 0;
			break;
		case HASH_DOWHILE:
		case HASH_WHILE:
			open[i] = 0;
			fprintf(out, "%sJMP _loop_%d\n%s_loop_%d_out:\n",
				tabs, curLOOP, tabs, curLOOP);
			popLOOP();
			break;
		case HASH_LOOP:
			open[i] = 0;
			fprintf(out, "%sLOOP _loop_%d\n%s_loop_%d_out:\n", 
				tabs, curLOOP, tabs, curLOOP);
			popLOOP();
			break;
		case HASH_FUNCTION:
			open[i] = 0;
			fprintf(out, "%s_%s_end:\n%sLEAVE\n%sRET\n%s_%s_out:\n", 
				tabs, func, tabs, tabs, tabs, func);
			sprintf(func, "0");
			break;
		case HASH_NONE:
		case HASH_END:
		default:
			ERROR();
	}
}
void openHash(int h){
	int i;
	for(i=0; i<OPEN_LEN; i++){
		if(open[i] == HASH_NONE){
			open[i] = h;
			return;
		}
	}
}
void trim(char* str){
	int i, len = strlen(str);
	for(i=len-1; isspace(str[i]); i--){
		str[i] = '\0';
	}
}
void getTabs(){
	int i, len=strlen(line);
	for(i=0; i<len; i++){
		if(!isspace(line[i])){
			tabs[i] = '\0';
			break;
		} else {
			tabs[i] = line[i];
		}
	}
}
bool printCond(){
	sscanf(line, " #%*[^ \t\n] %[^><!=\n] %[^ ] %[^\n]",
		v1, op, v2);
	trim(v1);
	trim(v2);
	trim(op);
	SWITCH(v1)
		CASE("true")
			return false;
		CASE("false")
			fprintf(out, "JMP ");
			return true;
	ENDSWITCH();

	SWITCH(op)
		CASE(">")
			sprintf(op, "LE");
		CASE(">=")
			sprintf(op, "L");
		CASE("==")
			sprintf(op, "NE");
		CASE("!=")
			sprintf(op, "E");
		CASE("<=")
			sprintf(op, "G");
		CASE("<")
			sprintf(op, "GE");
		DEFAULT()
			ERROR();
	ENDSWITCH()

	fprintf(out, "%sCMP %s, %s\n%sJ%s ", 
		tabs, v1, v2, tabs, op);
	return true;
}
int countIFs(){
	int i, count = 0;
	for(i=0; i<OPEN_LEN; i++){
		if(open[i] == HASH_IF){
			count++;
		}
	}
	return count;
}
void popIF(){
	int i, c;
	c = countIFs();
	subcountIF[c-1] = 0;
	for(i=AUX_LEN-1; i>=0; i--){
		if(stackIF[i] != 0){
			stackIF[i] = 0;
			if(i >= 1){
				curIF = stackIF[i-1];
			} else {
				curIF = 0;
			}
			return;
		}
	}
}
void pushIF(){
	int i, c;
	c = countIFs();
	subcountIF[c] = 1;
	for(i=0; i<AUX_LEN; i++){
		if(stackIF[i] == HASH_NONE){
			curIF = stackIF[i] = ++countIF;
			return;
		}
	}
	ERROR();
}
void popLOOP(){
	int i;
	for(i=AUX_LEN-1; i>=0; i--){
		if(stackLOOP[i] != 0){
			stackLOOP[i] = 0;
			if(i >= 1){
				curLOOP = stackLOOP[i-1];
			} else {
				curLOOP = 0;
			}
			return;
		}
	}
}
void pushLOOP(){
	int i;
	for (i=0; i<AUX_LEN; ++i){
		if(stackLOOP[i] == 0){
			curLOOP = stackLOOP[i] = ++countLOOP;
			return;
		}
	}
	ERROR();
}
bool getLine(){
	int i, c;
	if(fgets(line, 1000, in) == NULL){
		return false;
	}
	trim(line);
	getTabs();
	sscanf(line, " %s", temp);
	lineNumber++;
	if(temp[0] == '#'){
		sscanf(line, " #%[^ ]", temp);
		SWITCH(temp)
			CASE("end")
				closeHash();
			CASE("if")
				pushIF();
				fprintf(out, "%s_if_%d_%d:\n", 
					tabs, curIF, 1);
				if(printCond()){
					fprintf(out, "_if_%d_%d\n", 
						curIF, 2);
				}
				openHash(HASH_IF);
			CASE("elseif")
				if(curIF == 0){
					ERROR(); 
				}
				c = countIFs() - 1;
				subcountIF[c]++;
				fprintf(out, "%sJMP _if_%d_out\n%s_if_%d_%d:\n", 
					tabs, curIF, tabs, curIF, subcountIF[c]);
				if(printCond()){
					fprintf(out, "_if_%d_%d\n", 
						curIF, subcountIF[c]+1);
				}
				openHash(HASH_ELSEIF);
			CASE("else")
				if(curIF == 0){
					ERROR();
				}
				c = countIFs() - 1;
				subcountIF[c]++;
				fprintf(out, "%sJMP _if_%d_out\n%s_if_%d_%d:\n", 
					tabs, curIF, tabs, curIF, subcountIF[c]);
				openHash(HASH_ELSE);
			CASE("dowhile")
				pushLOOP();
				fprintf(out, "%sJMP _loop_%d_in\n%s_loop_%d:\n", 
					tabs, curLOOP, tabs, curLOOP);
				if(printCond()){
					fprintf(out, "_loop_%d_out\n", 
						curLOOP);	
				}
				fprintf(out, "%s_loop_%d_in:\n", 
					tabs, curLOOP);
				openHash(HASH_DOWHILE);
			CASE("while")
				pushLOOP();
				fprintf(out, "%s_loop_%d:\n", 
					tabs, curLOOP);
				if(printCond()){
					fprintf(out, "_loop_%d_out\n", 
						curLOOP);
				}
				openHash(HASH_WHILE);
			CASE("loop")
				pushLOOP();
				sscanf(line, " #%*[^ \t\n] %[^\n] ", temp);
				fprintf(out, "%sMOV ecx, %s\n%s_loop_%d:\n", 
					tabs, temp, tabs, curLOOP);
				openHash(HASH_LOOP);
			CASE("break")
				if(curLOOP){
					fprintf(out, "%sJMP _loop_%d_out\n", 
						tabs, curLOOP);
				} else {
					ERROR();
				}
			CASE("continue")
				if(curLOOP){
					fprintf(out, "%sJMP _loop_%d\n", 
						tabs, curLOOP);
				} else {
					ERROR();
				}
			CASE("return")
				sprintf(temp, "\\");
				sscanf(line, " #%*[^ \n\t] %[^\n] ", temp);
				SWITCH(temp)
					CASE("\\")
					DEFAULT()
						fprintf(out, "%sMOV [_temp], %s\n", 
							tabs, temp);
				ENDSWITCH()
				SWITCH(func)
					CASE("0")ERROR()
				ENDSWITCH()
				fprintf(out, "%sJMP _%s_end\n", tabs, func);
			CASE("function")
				int n, i;
				funcPushCount = 0;
				sscanf(line, " #%*[^ \n\t] %[^\n] ", 
					line);
				sscanf(line, " %[^ ] %d: %[^\n] ", 
					func, &n, line);
				fprintf(out, "%%macro %s %d\nPUSHA\n", 
					func, n);
				for(i=1; i<=n; i++){
					sscanf(line, " %[^,], %[^\n] ", 
						temp, line);
					fprintf(out, "%sMOV %s, %%%d\n", tabs, temp, i);
				}
				fprintf(out, "%sCALL _%s\n%sPOPA\n%%endmacro\nJMP _%s_out\n_%s:\n%sENTER 0, 0\n", 
					tabs, func, tabs, func, func, tabs);
				openHash(HASH_FUNCTION);
			CASE(">")
				sscanf(line, " #%*s %[^\n] ", temp);
				trim(temp);
				fprintf(out, "%sMOV %s, [_temp]\n", tabs, temp);
			DEFAULT()
				//printf("#");
				ERROR();
		ENDSWITCH()
	} else {
		fprintf(out, "%s\n", line);
	}
	return true;
}

int main(int argc, char* argv[]){
	int i, c;
	if(argc <= 1){
		printf("j-nasm: error: no input file specified\n");
		exit(0);
	} else {
		in = fopen(argv[1], "r");
	}
	out = fopen("./j.asm", "w");
	for(i = 0; i<OPEN_LEN; i++){
		open[i] = 0;
	}
	for (i = 0; i<AUX_LEN; i++){
		subcountIF[i] 	= 0;
		stackIF[i] 		= 0;
		stackLOOP[i]	= 0;
	}
	fprintf(out, ";\n; Little help from:\n; Joao Rietra\n;\n");
	while(getLine());
	fclose(in);
	fclose(out);
	printf("./j.asm\n");
	for(c = i = 0; i<OPEN_LEN; i++){
		if(open[i] != 0){
			if(!(c++)){
				printf("error: unclosed labels (missing #end):\n");
			}
			switch(open[i]){
				case HASH_IF:
					printf("#if\n"); 
					break;
				case HASH_ELSEIF:
					printf("#elseif\n");
					break;
				case HASH_ELSE:
					printf("#else\n");
					break;
				case HASH_WHILE:
					printf("#while\n");
					break;
				case HASH_DOWHILE:
					printf("#dowhile\n");
					break;
				case HASH_END:
				case HASH_NONE:
				default:
					break;
			}
		}
	}

	return 0;
}
