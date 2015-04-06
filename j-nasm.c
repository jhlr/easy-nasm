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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	stackLOOP[AUX_LEN];

char line[LINE_LEN], temp[20], func[50], line2[LINE_LEN];
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
			fprintf(out, "_if_%d_%d:\n", 
				curIF, ++subcountIF[c-1]);
		case HASH_ELSE:
			fprintf(out, "_if_%d_out:\n", 
				curIF);
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
			fprintf(out, "JMP _loop_%d\n_loop_%d_out:\n",
				curLOOP, curLOOP);
			popLOOP();
			break;
		case HASH_LOOP:
			open[i] = 0;
			fprintf(out, "LOOP _loop_%d\n_loop_%d_out:\n", 
				curLOOP, curLOOP);
			popLOOP();
			break;
		case HASH_FUNCTION:
			open[i] = 0;
			fprintf(out, "_%s_end:\nPOPA\nPUSH dword 0\nRET\n_%s_out:\n", 
				func, func);
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
	for(i=len-1; i>=0; i--){
		if(str[i]==' '){
			str[i] = '\0';
		} else {
			return;
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

	fprintf(out, "CMP %s, %s\nJ%s ", v1, v2, op);
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
	if(fscanf(in, " %[^\n]", line) == EOF){
		return false;
	}
	lineNumber++;
	if(line[0] == '#'){
		sscanf(&line[1], " %[^\n]", &line[1]);
		sscanf(line, "#%[^ \n\t]", temp);
		SWITCH(temp)
			CASE("end")
				closeHash();
			CASE("if")
				pushIF();
				fprintf(out, "_if_%d_%d:\n", 
					curIF, 1);
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
				fprintf(out, "JMP _if_%d_out\n_if_%d_%d:\n", 
					curIF, curIF, subcountIF[c]);
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
				fprintf(out, "JMP _if_%d_out\n_if_%d_%d:\n", 
					curIF, curIF, subcountIF[c]);
				openHash(HASH_ELSE);
			CASE("dowhile")
				pushLOOP();
				fprintf(out, "JMP _loop_%d_in\n_loop_%d:\n", 
					curLOOP, curLOOP);
				if(printCond()){
					fprintf(out, "_loop_%d_out\n", 
						curLOOP);	
				}
				fprintf(out, "_loop_%d_in:\n", 
					curLOOP);
				openHash(HASH_DOWHILE);
			CASE("while")
				pushLOOP();
				fprintf(out, "_loop_%d:\n", 
					curLOOP);
				if(printCond()){
					fprintf(out, "_loop_%d_out\n", 
						curLOOP);
				}
				openHash(HASH_WHILE);
			CASE("loop")
				pushLOOP();
				sscanf(line, " #%*[^ \t\n] %[^\n] ", temp);
				fprintf(out, "MOV ecx, %s\n_loop_%d:\n", 
					temp, curLOOP);
				openHash(HASH_LOOP);
			CASE("break")
				if(curLOOP){
					fprintf(out, "JMP _loop_%d_out\n", 
						curLOOP);
				} else {
					ERROR();
				}
			CASE("continue")
				if(curLOOP){
					fprintf(out, "JMP _loop_%d\n", 
						curLOOP);
				} else {
					ERROR();
				}
			CASE("return")
				sprintf(temp, "\\");
				sscanf(line, " #%*[^ \n\t] %[^\n] ", temp);
				SWITCH(temp)
					CASE("\\")
						fprintf(out, "JMP _%s_end\n", 
							func);
						return true;
				ENDSWITCH()
				SWITCH(func)
					CASE("0")ERROR()
				ENDSWITCH()
				fprintf(out, "MOV [_temp], %s\nPOPA\nPUSH dword[_temp]\nRET\n", temp);
			CASE("function")
				int n, i;
				sscanf(line, " #%*[^ \n\t] %[^\n] ", 
					line);
				sscanf(line, " %[^ ] %d: %[^\n] ", 
					func, &n, line2);
				sscanf(line2, " %[^\n] ", line);
				fprintf(out, "%%macro %s %d\nPUSHA\n", 
					func, n);
				for(i=1; i<=n; i++){
					sscanf(line, " %[^,], %[^\n] ", 
						temp, line);
					if(!strcmp(temp, "stack")){
						fprintf(out, "PUSH ");
					} else {
						fprintf(out, "MOV %s, ", temp);
					}
					fprintf(out, "%%%d\n", i);
				}
				fprintf(out, "CALL _%s\n%%endmacro\nJMP _%s_out\n_%s:\n", 
					func, func, func);
				openHash(HASH_FUNCTION);
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
