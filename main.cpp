#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <set>
#include <map>
#include <vector>
#include <iterator>
#include <stdio.h>
#include <time.h>

using namespace std;
//using namespace boost::algorithm;

#define bufferSize 1000000
#define StringLen 10

char Readbuffer[bufferSize]; // buffer for data load from the file
int CurrentState = -1;  // 0 for normal; 1 for other html label; 2 for script content; 3 for bad coding; -1 for header;
int Docs = 0;   //number of docs that has been processed;
//ofstream pWrite("result.txt",ios::out);
FILE* pWrite = fopen("result.txt","w"); // output result( the io is intended for the test on my own pc,
                        //this part needs to be modified later for data processing)

typedef struct word_Info{
        int index;
        int Freq;
        word_Info(int _index, int _Freq){
                index = _index; Freq = _Freq;
        }
}Info;

int glb_index = 0;      //current dictionary index for new words occurs.
map<string, Info> WordFreq;  // Preserve the content for dictionary.
set<string> StopWord;   // Preserve the list of stop words.

void PreserveDic(){      //Preserve the dictionary every now and then in case the accident occurs.
        ofstream Dictionary("dictionary.txt");
        map<string, Info>::iterator Dic_it;
        for(Dic_it = WordFreq.begin(); Dic_it != WordFreq.end(); Dic_it ++){
                Dictionary << Dic_it->first << "  "<< Dic_it ->second.index <<endl;
        }
}
void countFreq(string s){  //If it is already in the dictionary, count++; else add it into the dictionary. Set the Freq of stop words to be -1.
        //cout<<s<<" ";
        if(s.length() <= 2) return;
        map<string, Info>::iterator Dic_it;
        Dic_it = WordFreq.find(s);
        if(Dic_it == WordFreq.end()){
                if(StopWord.find(s) != StopWord.end()){
                         Info tmp(glb_index,-1);
                         WordFreq.insert(map<string,Info>::value_type(s,tmp));
                }
                else{
                         Info tmp(glb_index,1);
                         WordFreq.insert(map<string,Info>::value_type(s,tmp));
                }
                glb_index ++;
        }
        else{
                if(Dic_it->second.Freq != -1)
                        Dic_it->second.Freq = Dic_it->second.Freq + 1;
        }
}

void clearAndWrite(){   // output the result, reset the corresponding data structure.
        map<string,Info>::iterator Dic_it;
        vector<Info> OutInfo;
        vector<Info>::iterator Out_it;
        fprintf(pWrite,"%d ", 0);
        //pWrite<< Docs << " ";
        for(Dic_it = WordFreq.begin(); Dic_it != WordFreq.end();  Dic_it++){
                if(Dic_it -> second.Freq != 0 && Dic_it -> second.Freq != -1){
                         //pWrite <<  " " << Dic_it->second.index << ":" << Dic_it->second.Freq << " ";
                         fprintf(pWrite, " %d:%d ", Dic_it->second.index, Dic_it->second.Freq);
                         //OutInfo.push_back(Dic_it -> second);
                         //cout<<Dic_it ->first << " "<< Dic_it->second.index << " "<<Dic_it->second.Freq<<endl;
                        Dic_it -> second.Freq = 0;
                }
        }
        //pWrite << endl<< endl;
        fprintf(pWrite,"\n");
        //OutInfo.sort();
        //fprintf(pWrite,"%d ", 0);
        //for(Out_it = OutInfo.begin(); Out_it != OutInfo.end(); Out_it ++){
        //        fprintf(pWrite, " %d:%d ", Out_it->index, Out_it->Freq);
        //}
        //fprintf(pWrite,"\n");
}

bool detect(int &ptr, char* goal){ // test whether the html label belongs to certain type.
        int tmp = ptr+1;
        while(Readbuffer[tmp] == ' ' || Readbuffer[tmp] == '\t' )
                tmp ++;
        if(strlen(Readbuffer) - tmp < strlen(goal)) return false;
        char TestChar[StringLen];
        memset(TestChar, 0, sizeof(TestChar));
        strncpy(TestChar, Readbuffer+tmp, strlen(goal));
        if(strcasecmp(goal,TestChar) == 0){
                ptr = tmp + strlen(goal) -1;
                return true;
        }
        return false;
}

//extract the valid words from the html source code. Processing one line each time.
void analysisLine(){
        bool wordBegin = false;
        char words[100];
        int wordPtr = 0;
        string ValidWords;
        memset(words, 0, sizeof(words));
        for(int i = 0; Readbuffer[i] != '\0' ; i++ ){
                switch(CurrentState){ // 0 for normal; 1 for other html label; 2 for script content; 3 for bad coding; -1 for header;
                        case -1:
                                if(Readbuffer[i] == '<' ){
                                        if(detect(i,"html")){
                                                CurrentState = 1;
                                        }
                                }
                                break;
                        case 0:
                                if(Readbuffer[i] >= 'A' && Readbuffer[i] <='Z'){
                                            Readbuffer[i] = Readbuffer[i] - 'A' + 'a';
                                }
                                if((Readbuffer[i] >= 'a' && Readbuffer[i] <= 'z')){
                                            words[wordPtr++] = Readbuffer[i];
                                            wordBegin = true;
                                }
                                else{
                                          if(Readbuffer[i] == '<' ){
                                                 if(detect(i,"script"))         CurrentState = 2;
                                                 else if(detect(i,"!--"))       CurrentState = 4;
                                                 else CurrentState = 1;
                                          }
                                          if(Readbuffer[i] == '&'){
                                                        CurrentState = 3;
                                          }
                                          if(wordBegin){
                                                words[wordPtr] = '\0';
                                                ValidWords.assign(words);
                                                countFreq(ValidWords);
                                                wordPtr = 0;    wordBegin = false;
                                          }
                                }
                                break;
                        case 1:
                                if(Readbuffer[i] == '/'){
                                        if(detect(i,"html") && Readbuffer[i+1] == '>'){
                                                        CurrentState = -1;
                                                        i++;
                                        }
                                }
                                else if(Readbuffer[i] == '>')
                                        CurrentState = 0;
                                break;
                        case 2:
                                if(Readbuffer[i] == '/'){
                                        if(detect(i,"script") && Readbuffer[i+1] == '>'){
                                                        CurrentState = 0;
                                                        i++;
                                        }
                                }
                                break;
                        case 3:
                                if(Readbuffer[i] == ' ' || Readbuffer[i] == '\t')       CurrentState = 0;
                                if(Readbuffer[i] == '<' ){
                                                if(detect(i,"script"))  CurrentState = 2;
                                                else CurrentState = 1;
                                }
                                break;
                        case 4:
                                if(Readbuffer[i] == '-'){
                                                if(detect(i,"->"))      CurrentState = 0;
                                }
                }
        }
}

//Filtering the web header ,html tags and script of the documents. Deal with one line each time.
void extractWords(){
        ifstream InputFile("0013wb-88.txt");
        //ifstream InputFile("testInput.txt");
        while(InputFile.getline(Readbuffer,bufferSize)){
                Readbuffer[strlen(Readbuffer) - 1] = '\0';
                if(strcmp(Readbuffer,"WARC/1.0") == 0){
                        CurrentState = -1;
                        Docs ++;
                        if(Docs % 100 == 0){
                                cout<< "Processed " << Docs << "files"<<endl;
                        }
                        if(Docs % 10000 == 0){
                                PreserveDic();
                        }
                        clearAndWrite();
                        continue;
                }
                //if(strlen(Readbuffer) != 0){
                        analysisLine();
                //}
        }
        cout<< "Processed " << Docs << "files"<<endl;
        clearAndWrite();
}

int main()
{
    //freopen("Docs.txt","w",stdout);
    ifstream StpWrdFile("stopWords.txt");
    string stopW;
    clock_t start = clock();
    while(StpWrdFile.getline(Readbuffer,bufferSize)){   // load the stop words.
                Readbuffer[strlen(Readbuffer) - 1] = '\0';
                stopW.assign(Readbuffer);
                StopWord.insert(stopW);
    }
    extractWords();     // filtering the html label and process the words.
    clock_t stop = clock();
    cout<<"The conversion is finished!"<<endl;
    cout<<"Total time is "<< (double)(stop - start) / CLOCKS_PER_SEC <<" secs" << endl;
    return 0;
}
