//  Created by 王方 on 05/12/2018.
//  Copyright © 2018 王方. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <algorithm>
#include <queue>
#include <vector>

using namespace std;
#define DatasetVolume 269646
#define DIMENSION_NUM 144
#define KNN_NUM 10
#define SCALEBIT 4
#define QUERY_NUM 2
struct ObjectInData{
    int TagNO;
    double L2NormBound;
};

struct ResultArray{
    int TagNO;
    double L2NormExact;
};

struct cmp{
    bool operator()(ObjectInData a, ObjectInData b){
        return a.L2NormBound<b.L2NormBound;
    }
};

bool Cmpare_R(const ResultArray &a, const ResultArray &b)
{
    return a.L2NormExact<b.L2NormExact;
}

void MinK(struct ObjectInData Obj[], int n, int k, struct ResultArray res[]); // Function for find K smallest
double Exact_L2Norm_Comp(int Object_no, double Query_element[]);
double *cacth_query_point(double query_point[],int no);
double *lower_bound_comp(double lower_bound[],double query_point[]);


int main(int argc, const char * argv[]) {
    
    printf("This Code Measure Time Cost During Exact Computation Phase \n");
    printf("Which contains three parts:\n \n");
    printf("Part1                          | Part2              | Part3 \n");
    printf("Finding K Smallest Lower bound | Exact L2-norm Comp | Prune Objects \n \n");
    struct timeval tv1,tv2;
    struct timeval tv3,tv4;
    struct timeval tv5,tv6;
    double part1_time=0;
    double part3_time=0;
    double part2_time=1.5;
    int update_count=0;
    
    //Repeat Experiment, Repaeating time is QUERY_NUM
    for(int z=0; z<QUERY_NUM;z++){
        double global_time=0;
        double query_point[144]={0};
        struct  ObjectInData DataObject[DatasetVolume];
        struct  ResultArray Resultarray[KNN_NUM];
        double lower_bound[DatasetVolume]={0};
        cacth_query_point(query_point, z);
        lower_bound_comp(lower_bound, query_point);
        for(int k=0; k<DatasetVolume; k++)
        {
            DataObject[k].TagNO=k;
            DataObject[k].L2NormBound=lower_bound[k];
        }
        //Ingore above code, as we focus on exact computation stage
        //Acode is the simple example of pre-process and ReRAM PIM stages
        
        
        //3.1 Find K smallest lower bound
        gettimeofday(&tv1,NULL);
        MinK(DataObject, DatasetVolume, KNN_NUM,Resultarray);
        gettimeofday(&tv2,NULL);
        part1_time+=tv2.tv_sec - tv1.tv_sec + (tv2.tv_usec - tv1.tv_usec)/1000000.0;
        
        
        //3.2 Exact computation for K objects
        for(int j=0; j<KNN_NUM; j++)
        {
            int temp_tagno=Resultarray[j].TagNO;
            Resultarray[j].L2NormExact=Exact_L2Norm_Comp(temp_tagno, query_point);
        }
        
        //3.3 Prune unpromising objects (might exact computation)*/
        int Alredy_result_Flag=0;
        gettimeofday(&tv3,NULL);
        sort(Resultarray, Resultarray+KNN_NUM, Cmpare_R);
        for(int no=0; no<DatasetVolume; no++)
        {
            
            Alredy_result_Flag=0;
            if(Resultarray[KNN_NUM-1].L2NormExact >= DataObject[no].L2NormBound)
            {
                for(int j=0; j<KNN_NUM;j++)
                {
                    if(Resultarray[j].TagNO==DataObject[no].TagNO)
                    {
                        Alredy_result_Flag=1;
                    }
                }
                if(Alredy_result_Flag==0)
                {
                    gettimeofday(&tv5,NULL);
                    double L2NormExact_no=Exact_L2Norm_Comp(DataObject[no].TagNO, query_point);
                    gettimeofday(&tv6,NULL);
                    if(Resultarray[KNN_NUM-1].L2NormExact >= L2NormExact_no)
                    {
                        Resultarray[KNN_NUM-1].L2NormExact=L2NormExact_no;
                        Resultarray[KNN_NUM-1].TagNO=DataObject[no].TagNO;
                        sort(Resultarray, Resultarray+KNN_NUM, Cmpare_R);
                        update_count++;
                    }
                    global_time+=tv6.tv_sec - tv5.tv_sec + (tv6.tv_usec - tv5.tv_usec)/1000000.0;
                }
            }
        }
        gettimeofday(&tv4,NULL);
        part3_time+=(tv4.tv_sec - tv3.tv_sec + (tv4.tv_usec - tv3.tv_usec)/1000000.0)-global_time;
        
    
    }
    
    
    part1_time=part1_time/(QUERY_NUM);
    part3_time=part3_time/(QUERY_NUM);
    part2_time=part2_time*((update_count)/(QUERY_NUM)+KNN_NUM);

    printf("Experiment Background: K=10   Number of random query=50 \n \n");
    

    printf("Experiment Result: \n");
    printf("Part1                          | Part2              | Part3 \n");
    printf("%lf us                  %lf us            %lf us \n \n",part1_time*pow(10,6),part2_time,part3_time*pow(10,6));
    
    
    
   
    printf("Unsuccessful pruned objects: %d",update_count);
    return 0;
}


void MinK(struct ObjectInData Obj[], int n, int k, struct ResultArray res[])
{
    priority_queue <ObjectInData, vector<ObjectInData>,cmp> pq;
    for(int i = 0;i < k;i++)
        pq.push(Obj[i]);
    for(int i = k;i < n;i++)
        if(Obj[i].L2NormBound <pq.top().L2NormBound){
            pq.pop();
            pq.push(Obj[i]);
        }
    for(int i = 0; i < k;i++){
        res[k-1-i].L2NormExact = pq.top().L2NormBound;
        res[k-1-i].TagNO = pq.top().TagNO;
        pq.pop();
    }
}












//Below Function are used for Pre-processing or Exact Distance Comp
double Exact_L2Norm_Comp(int Object_no, double Query_element[]){

    double Object_L2Norm=0;
    FILE *fp=fopen("Normalized_CORR.dat", "r");
    char *p;
    if (NULL == fp) {
        printf("CAN NOT OPEN THE FILE\n");
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if(size > 0)
    {
        p = (char *)calloc(size,sizeof(char));
    }
    fread(p, size, 1, fp);
    fclose(fp);
    p[size-1] = '\0';
    
    int line_no=0;
    stringstream target_line;
    for(int k=0; k<size; k++)
    {
        if(p[k]=='\n')
        {   line_no++; }
        if(line_no==Object_no)
            target_line<<p[k];
    }
    vector<string> arr1;
    string word;
    while(target_line>>word) {
        arr1.push_back(word);
    }
    struct timeval tv5,tv6;
    gettimeofday(&tv5,NULL);
    for(size_t i=0; i<arr1.size(); i++) {
        double Object_Orginal= stod(arr1[i]);
        Object_L2Norm+=(Object_Orginal-Query_element[i])*(Object_Orginal-Query_element[i]);
    }
    Object_L2Norm=Object_L2Norm*100000000;
    gettimeofday(&tv6,NULL);
    free(p);
    return Object_L2Norm;
}


double *cacth_query_point(double query_point[],int no){
    FILE *fp_q=fopen("Query_set_50.dat", "r");
    char *q;
    if (NULL == fp_q) {
        printf("CAN NOT OPEN THE FILE\n");
    }
    fseek(fp_q, 0, SEEK_END);
    long size = ftell(fp_q);
    fseek(fp_q, 0, SEEK_SET);
    if(size > 0)
    {
        q = (char *)calloc(size,sizeof(char));
    }
    fread(q, size, 1, fp_q);
    fclose(fp_q);
    q[size-1] = '\0';
    
    int line_no=0;
    stringstream target_line;
    for(int k=0; k<size; k++)
    {
        if(q[k]=='\n')
        {   line_no++; }
        if(line_no==no)
            target_line<<q[k];
    }
    vector<string> arr1;
    string word;
    while(target_line>>word) {
        arr1.push_back(word);
    }
    for(size_t i=0; i<arr1.size(); i++) {
        double Object_Orginal_element= stod(arr1[i]);
        query_point[i]=Object_Orginal_element;
    }
    free(q);
    fclose(fp_q);
    return query_point;
}


double *lower_bound_comp(double lower_bound[],double query_point[]){
    
    double query_pow2=0;
    double query_pow1=0;
    int query_int[DIMENSION_NUM]={0};
    for(int i=0;i<DIMENSION_NUM;i++)
    {
        query_pow2+=pow((query_point[i]*pow(10, SCALEBIT)+1*pow(10, SCALEBIT)),2);
        query_int[i]=int(query_point[i]*pow(10, SCALEBIT)+1*pow(10, SCALEBIT));
        query_pow1+=int(query_point[i]*pow(10, SCALEBIT)+1*pow(10, SCALEBIT));
    }
    
    FILE *fp_q=fopen("Normalized_CORR.dat", "r");
    char *q;
    if (NULL == fp_q) {
        printf("CAN NOT OPEN THE FILE\n");
    }
    fseek(fp_q, 0, SEEK_END);
    long size = ftell(fp_q);
    fseek(fp_q, 0, SEEK_SET);
    if(size > 0)
    {
        //printf("size: %ld\n",size);
        q = (char *)calloc(size,sizeof(char));
    }
    fread(q, size, 1, fp_q);
    fclose(fp_q);
    q[size-1] = '\0';
    
    int line_no=0;
    stringstream line_element;
    for(int k=0; k<size; k++)
    {
        if(q[k]=='\n')
        {
            double object_pow1=0;
            double object_pow2=0;
            double object_pim=0;
            vector<string> arr1;
            string word;
            while(line_element>>word) {
                arr1.push_back(word);
            }
            for(size_t i=0; i<arr1.size(); i++) {
                double Object_Orginal_element= stod(arr1[i]);
                object_pow2+=pow((Object_Orginal_element*pow(10, SCALEBIT)+1*pow(10, SCALEBIT)),2);
                object_pow1=int(Object_Orginal_element*pow(10, SCALEBIT)+1*pow(10, SCALEBIT));
                object_pim+=int(Object_Orginal_element*pow(10, SCALEBIT)+1*pow(10, SCALEBIT))*query_int[i];
                }
            lower_bound[line_no]=object_pow2-2*object_pow1+object_pim+query_pow2-2*query_pow1+2*DIMENSION_NUM;
            line_element.clear();
            line_no++;
        }
        
        else {
            line_element<<q[k];
        }
    }
    free(q);
    fclose(fp_q);
    return lower_bound;
}
