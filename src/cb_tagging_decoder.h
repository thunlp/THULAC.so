#pragma once
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <list>
#include "thulac_base.h"
#include "dat.h"
#include "cb_ngram_feature.h"
#include "cb_decoder.h"
#include "cb_model.h"


namespace thulac{


class TaggingDecoder{
public:

    ///*模型参数*/
    permm::Model* model;

    DAT* dat;
    char** label_info;
    int** pocs_to_tags;


    char separator;

    int max_length;
    /*句子*/
    int len;//句子长度
    int* sequence;//句子
    int** allowed_label_lists;///

    ///*特征*//
    NGramFeature* ngram_feature;

    ///*解码用*/
    permm::Node* nodes;//只用来存储解码用的节点的拓扑结构
    int* values;//存各个节点的权重
    permm::Alpha_Beta* alphas;//前向算法数据
    permm::Alpha_Beta* betas;//后向算法数据
    int best_score;
    int* result;//存储结果

    ///**合法转移矩阵*/
    int*label_trans;
    int**label_trans_pre;
    int**label_trans_post;

    ///*后处理用*/
    int threshold;
    int* allow_com;

    ///*后处理用_ tagging*/
    int tag_size;//postag的个数
    int** label_looking_for;
    int* is_good_choice;
    
    /*构造函数*/
    TaggingDecoder();
    ~TaggingDecoder();
    
    
    /*初始化*/
    void init(permm::Model* model, DAT* dat, char** label_info, int** pocs_to_tags,
        char* label_trans=NULL);
    void set_label_trans();//
   
    /*解码*/
    void put_values();
    void dp();
    void cal_betas();
    void cal_betas(int**);
    
    /*接口*/
    int segment(int* input,int length,int* tags);
    int segment(RawSentence&);
    int segment(RawSentence&,SegmentedSentence&);
    int segment(RawSentence&,POCGraph&,SegmentedSentence&);
    int segment(RawSentence&,POCGraph&,TaggedSentence&);
    int segment(RawSentence&,POCGraph&);
    int segment(RawSentence&,POCGraph&, POCGraph&);
    int segment(POCGraph&,RawSentence&);
    int segment(int**,RawSentence&);
    int segment(int**, RawSentence&, TaggedSentence&);
    int segment(RawSentence&,POCGraph&,Lattice&);
    
    void find_good_choice();
    void generate_lattice(Lattice&);
    
    void get_result(TaggedSentence&);
    void get_seg_result(SegmentedSentence& ss);
    /*输入输出*/
    void output_raw_sentence();
    void output_sentence();
    void output_allow_tagging();
    
private:
    void load_label_trans(char*filename);
};




TaggingDecoder::TaggingDecoder(){
    this->separator='_';
    this->max_length=10000;
    this->len=0;
    this->sequence=new int[this->max_length];
    this->allowed_label_lists=new int*[this->max_length];

    ngram_feature=NULL;
    
    nodes=new permm::Node[this->max_length];
    
    this->label_trans=NULL;
    label_trans_pre=NULL;
    label_trans_post=NULL;
    this->threshold=0;
    
//    this->allow_sep=new int[this->max_length];
    this->allow_com=new int[this->max_length];
    
    this->tag_size=0;
    //this->is_good_choice=NULL;
    
    alphas=NULL;
    betas=NULL;
    
}
TaggingDecoder::~TaggingDecoder(){
    delete[]sequence;
    delete[]allowed_label_lists;
    
    delete ngram_feature;
    
    for(int i=0;i<max_length;i++){
        delete[](nodes[i].predecessors);
        delete[](nodes[i].successors);
    }
    delete[](nodes);
    free(values);
    free(alphas);
    free(betas);
    free(result);
    
        
    
    delete[](label_trans);
    if(model!=NULL)for(int i=0;i<model->l_size;i++){
        if(label_trans_pre)delete[](label_trans_pre[i]);
        if(label_trans_post)delete[](label_trans_post[i]);
    }
    delete[](label_trans_pre);
	delete[](label_trans_post);
    
//    delete[](allow_sep);
    delete[](allow_com);
    
	if(model!=NULL)for(int i=0;i<model->l_size;i++){
		if(label_looking_for)delete[](label_looking_for[i]);
	};
	delete[](label_looking_for);
    delete[](is_good_choice);
    
}

void TaggingDecoder::init(
        permm::Model* model,
        DAT* dat,
        char** label_info,
        int** pocs_to_tags,
        char* label_trans
        ){
    /**模型*/
    this->model = model;
    
    /**解码用*/
    values=(int*)calloc(sizeof(int),max_length*model->l_size);
    alphas=(permm::Alpha_Beta*)calloc(sizeof(permm::Alpha_Beta),max_length*model->l_size);
    betas=(permm::Alpha_Beta*)calloc(sizeof(permm::Alpha_Beta),max_length*model->l_size);
    result=(int*)calloc(sizeof(int),max_length*model->l_size);
    this->label_info=label_info;
    
    for(int i=0;i<max_length;i++){
        int* pr=new int[2];
        pr[0]=i-1;
        pr[1]=-1;
        nodes[i].predecessors=pr;
        
        pr=new int[2];
        pr[0]=i+1;
        pr[1]=-1;
        nodes[i].successors=pr;
    };
    
    //DAT
    this->dat=dat;

    //Ngram Features
    ngram_feature=new NGramFeature(dat,model,values);

    /*pocs_to_tags*/
    this->pocs_to_tags=pocs_to_tags;
    
    label_looking_for=new int*[model->l_size];
    for(int i=0;i<model->l_size;i++)
        label_looking_for[i]=NULL;
    for(int i=0;i<model->l_size;i++){
        if(label_info[i][0]==kPOC_B || label_info[i][0]==kPOC_S)continue;
        
        for(int j=0;j<=i;j++){
            if((strcmp(label_info[i]+1,label_info[j]+1)==0)&&(label_info[j][0]==kPOC_B)){
                if(label_looking_for[j]==NULL){
                    label_looking_for[j]=new int[2];
                    label_looking_for[j][0]=-1;label_looking_for[j][1]=-1;
                    tag_size++;
                }
                label_looking_for[j][label_info[i][0]-'1']=i;
                break;
            }
        }
    }
    //printf("tagsize %d",tag_size);
    

    
    /**label_trans*/
    if(label_trans){
        load_label_trans(label_trans);
    }
    
   for(int i=0;i<max_length;i++)
       allowed_label_lists[i]=NULL;
    
    is_good_choice=new int[max_length*model->l_size];
    
}
void TaggingDecoder::dp(){
    if(allowed_label_lists[0]==NULL){
        allowed_label_lists[0]=pocs_to_tags[9];
    }
    if(allowed_label_lists[len-1]==NULL){
        allowed_label_lists[len-1]=pocs_to_tags[12];
    }
    int* p=allowed_label_lists[0];
    while(*p!=-1){
        //std::cout<<"p "<<*p<<"\n";
        p++;
    }
    best_score=dp_decode(
            model->l_size,//check
            model->ll_weights,//check
            len,//check
            nodes,
            values,
            alphas,
            result,
            label_trans_pre,
            allowed_label_lists
        );
    //std::cout<<best_score<<"\n";
    allowed_label_lists[0]=NULL;
    allowed_label_lists[len-1]=NULL;
    /*for(int i=0;i<len;i++){
        printf("%s",label_info[result[i]]);

        std::cout<<" ";
    }std::cout<<"\n";*/
}
void TaggingDecoder::cal_betas(){
    if(allowed_label_lists[0]==NULL){
        allowed_label_lists[0]=pocs_to_tags[9];
    }
    if(allowed_label_lists[len-1]==NULL){
        allowed_label_lists[len-1]=pocs_to_tags[12];
    }
    int tmp=nodes[len-1].successors[0];
    nodes[len-1].successors[0]=-1;
    dp_cal_betas(
            model->l_size,
            model->ll_weights,
            len,
            nodes,
            values,
            betas,
            label_trans_post,
            allowed_label_lists
    );
    nodes[len-1].successors[0]=tmp;
    allowed_label_lists[0]=NULL;
    allowed_label_lists[len-1]=NULL;

}

void TaggingDecoder::cal_betas(int** tags){
    for(int i=0;i<len;i++){
        allowed_label_lists[i]=tags[i];
    }

    int tmp=nodes[len-1].successors[0];
    nodes[len-1].successors[0]=-1;
    dp_cal_betas(
            model->l_size,
            model->ll_weights,
            len,
            nodes,
            values,
            betas,
            label_trans_post,
            allowed_label_lists
    );
    nodes[len-1].successors[0]=tmp;

}

void TaggingDecoder::set_label_trans(){
    int l_size=this->model->l_size;
    // std::list<int> pre_labels[l_size];
    std::list<int> *pre_labels = new std::list<int>[l_size];
    // std::list<int> post_labels[l_size];
    std::list<int> *post_labels = new std::list<int>[l_size];
    for(int i=0;i<l_size;i++)
        for(int j=0;j<l_size;j++){
            // 0:B 1:M 2:E 3:S
            int ni=this->label_info[i][0]-'0';
            int nj=this->label_info[j][0]-'0';
            int i_is_end=((ni==2)//i is end of a word
                    ||(ni==3));
            int j_is_begin=((nj==0)//i is end of a word
                    ||(nj==3));
            int same_tag=strcmp(this->label_info[i]+1,this->label_info[j]+1);
            
            if(same_tag==0){
                if((ni==0&&nj==1)||
                        (ni==0&&nj==2)||
                        (ni==1&&nj==2)||
                        (ni==1&&nj==1)||
                        (ni==2&&nj==0)||
                        (ni==2&&nj==3)||
                        (ni==3&&nj==3)||
                        (ni==3&&nj==0)){
                    pre_labels[j].push_back(i);
                    post_labels[i].push_back(j);
                    //printf("ok\n");
                }
            }else{
                //printf("%s <> %s\n",this->label_info[i],this->label_info[j]);
                if(i_is_end&&j_is_begin){
                    pre_labels[j].push_back(i);
                    post_labels[i].push_back(j);
                }
            }
        }
    label_trans_pre=new int*[l_size];
    for(int i=0;i<l_size;i++){
        label_trans_pre[i]=new int[(int)pre_labels[i].size()+1];
        int k=0;
        for(std::list<int>::iterator plist = pre_labels[i].begin();
                plist != pre_labels[i].end(); plist++){
            label_trans_pre[i][k]=*plist;
            k++;
        };
        label_trans_pre[i][k]=-1;
    }
    label_trans_post=new int*[l_size]; 
    for(int i=0;i<l_size;i++){
        label_trans_post[i]=new int[(int)post_labels[i].size()+1];
        int k=0;
        for(std::list<int>::iterator plist=post_labels[i].begin();
                plist!=post_labels[i].end();++plist){
            label_trans_post[i][k]=*plist;
            k++;
        };
        label_trans_post[i][k]=-1;
    }
    delete [] post_labels;
    delete [] pre_labels;
};

void TaggingDecoder::load_label_trans(char*filename){
    //打开文件
    FILE * pFile=fopen ( filename , "rb" );
    /*得到文件大小*/
    int remain_size=0;
    int rtn=fread (&remain_size,sizeof(int),1,pFile);
    /*得到矩阵数据*/
    label_trans=new int[remain_size];
    rtn=fread (label_trans,sizeof(int),remain_size,pFile);
    
    /*计算标签个数*/
    int label_size=0;
    for(int i=0;i<remain_size;i++){
        if(label_trans[i]==-1)label_size++;
    }
    label_size/=2;
    /*设定各个标签的指针*/
    label_trans_pre=new int*[label_size];
    label_trans_post=new int*[label_size];
    int ind=0;
    for(int i=0;i<label_size;i++){
        label_trans_pre[i]=label_trans+ind;
        while(label_trans[ind]!=-1)ind++;ind++;
        label_trans_post[i]=label_trans+ind;
        while(label_trans[ind]!=-1)ind++;ind++;
    }
    fclose (pFile);
    return;
}

void TaggingDecoder::put_values(){
    if(!len)return;
    /*nodes*/
    for(int i=0;i<len;i++){
        nodes[i].type=0;
    }
    nodes[0].type+=1;
    nodes[len-1].type+=2;
    /*values*/
    memset(values,0,sizeof(*values)*len*model->l_size);
    ngram_feature->put_values(sequence,len);
}


void TaggingDecoder::output_raw_sentence(){
    int c;
    for(int i=0;i<len;i++){
        thulac::put_character(sequence[i]);
        
    }
}
void TaggingDecoder::output_sentence(){
    int c;
    for(int i=0;i<len;i++){
        thulac::put_character(sequence[i]);
        
        if((i==len-1)||(label_info[result[i]][0]==kPOC_E)||(label_info[result[i]][0]==kPOC_S)){//分词位置
            if(*(label_info[result[i]]+1)){//输出标签（如果有的话）
                putchar(separator);
                printf("%s",label_info[result[i]]+1);
            }
            if((i+1)<len)putchar(' ');//在分词位置输出空格
        }
    }
}
void TaggingDecoder::get_result(TaggedSentence& ts){
    int c;
    int offset=0;
    ts.clear();
    for(int i=0;i<len;i++){
        if((i==len-1)||(label_info[result[i]][0]==kPOC_E)||(label_info[result[i]][0]==kPOC_S)){//分词位置
            ts.push_back(WordWithTag(separator));
            for(int j=offset;j<i+1;j++){
                ts.back().word.push_back(sequence[j]);
            }
            offset=i+1;
            if(*(label_info[result[i]]+1)){//输出标签（如果有的话）
                ts.back().tag=label_info[result[i]]+1;
                //printf("%s",label_info[result[i]]+1);
            }
            //if((i+1)<len)putchar(' ');//在分词位置输出空格
        }
    }
};

void TaggingDecoder::get_seg_result(SegmentedSentence& ss){
        ss.clear();
        /*
        Raw raw;
        for(int i = 0; i < len; i ++){
                raw.push_back(sequence[i]);
        }
        std::cerr<<raw<<std::endl;
        */
    for(int i=0;i<len;i++){
        if((i==0)||(label_info[result[i]][0]==kPOC_B)||(label_info[result[i]][0]==kPOC_S)){
            ss.push_back(Word());
        }
        ss.back().push_back(sequence[i]);
    }
};

void TaggingDecoder::find_good_choice(){
    /*找出可能的标注*/
    for(int i=0;i<len*model->l_size;i++){
        is_good_choice[i]=false;
    }
    for(int i=0;i<len;i++){
        int* p_allowed_label=allowed_label_lists[i];
        int j=-1;
        //std::cout<<p_allowed_label<<" ";
        while((p_allowed_label?
                    ((j=(*(p_allowed_label++)))!=-1)://如果有指定，则按照列表来
                    ((++j)!=model->l_size))){//否则枚举
            int ind=i*model->l_size+j;
            is_good_choice[ind]=
                alphas[ind].value+betas[ind].value-values[ind]+threshold>=best_score;
        }

    }
    for(int i=0;i<len*model->l_size;i++){
        
        //is_good_choice[i]=alphas[i].value+betas[i].value-values[i]+threshold>=best_score;
    }
};

void TaggingDecoder::generate_lattice(Lattice& lattice){
    lattice.clear();
    find_good_choice();
    /*找出可能的词*/
    int this_score=0;
    int left_part=0;
    int last_id=0;
	//std::cerr<<"len:"<<len<<std::endl;
    for(int i=0;i<len;i++){
        for(int b_label_i=0;b_label_i<model->l_size;b_label_i++){
            if(!is_good_choice[i*model->l_size+b_label_i]){
                continue;
            }
            //std::cout<<i<<" "<<b_label_i<<"\n";
            //#if((label_info[b_label_i][0]==kPOC_S)||(i==0&&label_info[b_label_i][0]==kPOC_E)){
            if((label_info[b_label_i][0]==kPOC_S)){
                //输出单个字的词
                this_score=alphas[i*model->l_size+b_label_i].value
                        +betas[i*model->l_size+b_label_i].value
                        -values[i*model->l_size+b_label_i];
                //printf("%d,%d,%s,%d ",i,i+1,label_info[b_label_i]+1,best_score-this_score);
                lattice.push_back(LatticeEdge());
                LatticeEdge& edge=lattice.back();
                edge.begin=i;
                edge.word=Raw();edge.word.push_back(sequence[i]);
                edge.tag=std::string(label_info[b_label_i]+1);
                edge.margin=best_score-this_score;

            }else if(label_info[b_label_i][0]==kPOC_B){
                int mid_ind=label_looking_for[b_label_i][0];
                int right_ind=label_looking_for[b_label_i][1];
                left_part=alphas[i*model->l_size+b_label_i].value;
                last_id=b_label_i;
                for(int j=i+1;j<len;j++){
                    if(j==len)break;
                    if(right_ind==-1)break;

                    if((is_good_choice[j*model->l_size+right_ind])){
                        //check，是不是合格的词
                        this_score=left_part
                                +model->ll_weights[last_id*model->l_size+right_ind]
                                +betas[j*model->l_size+right_ind].value;
                        if(best_score-this_score<=threshold){
                            //printf("%d,%d,%s,%d ",i,j+1,label_info[b_label_i]+1,best_score-this_score);
                            lattice.push_back(LatticeEdge());
                            LatticeEdge& edge=lattice.back();
                            edge.begin=i;
                            edge.word=Raw();
                            for(int k=i;k<j+1;k++)
                                edge.word.push_back(sequence[k]);
                            edge.tag=std::string(label_info[b_label_i]+1);
                            edge.margin=best_score-this_score;

                        }
                    }
                    if(mid_ind==-1)break;
                    if(!is_good_choice[(j*(model->l_size))+mid_ind])
                        break;
                    left_part+=values[j*model->l_size+mid_ind]
                            +model->ll_weights[last_id*model->l_size+mid_ind];
                    last_id=mid_ind;
                }
                
            }
        }
    }
	std::cerr<<"lattice:"<<lattice.size()<<std::endl;
};
    
void TaggingDecoder::output_allow_tagging(){
    Lattice lattice;
    generate_lattice(lattice);
    for(int i=0;i<lattice.size();i++){
        LatticeEdge& edge=lattice[i];
        printf("%d,%d,%s,%d ",edge.begin,edge.begin+(int)edge.word.size(),edge.tag.c_str(),edge.margin);
    }
    //std::cout<<lattice<<"\n";
    return;
}

int TaggingDecoder::segment(int* input,int length,int* tags){
    if(not length)return 0;
    for(int i=0;i<length;i++){
        sequence[i]=input[i];
    }
    len=length;
    
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里
    
    for(int i=0;i<len;i++){
        if((label_info[result[i]][0]==kPOC_B)||(label_info[result[i]][0]==kPOC_S)){//分词位置
            tags[i]=1;
        }else{
            tags[i]=0;
        }
    }
}

int TaggingDecoder::segment(RawSentence& raw){
    if(raw.size()==0)return 0;
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里
}

int TaggingDecoder::segment(RawSentence& raw,SegmentedSentence& segged){
    segged.clear();
    if(raw.size()==0)return 0;
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里
    for(int i=0;i<len;i++){
        if((i==0)||(label_info[result[i]][0]==kPOC_B)||(label_info[result[i]][0]==kPOC_S)){
            segged.push_back(Word());
        }
        segged.back().push_back(raw[i]);
    }
}

int TaggingDecoder::segment(RawSentence& raw,
        POCGraph&old_graph,
        SegmentedSentence& segged){
    for(int i=0;i<(int)raw.size();i++){
        int pocs=old_graph[i];
        //std::cout<<pocs<<" ";
        if(pocs){
            allowed_label_lists[i]=pocs_to_tags[pocs];
        }else{
            allowed_label_lists[i]=pocs_to_tags[15];
        }
    }
    //std::cout<<"\n";
    segment(raw,segged);
    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=NULL;
    }
}

int TaggingDecoder::segment(RawSentence& raw, POCGraph& graph, TaggedSentence& ts){
    if(raw.size()==0)return 0;

    for(int i=0;i<(int)raw.size();i++){
        int pocs = graph[i];
        if(pocs){
            allowed_label_lists[i]=pocs_to_tags[pocs];
        }else{
            allowed_label_lists[i]=pocs_to_tags[15];
        }
    }
    //std::cout<<"\n";
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=NULL;
    }

    int c;
    int offset=0;
    ts.clear();
    for(int i=0;i<len;i++){
        if((i==len-1)||(label_info[result[i]][0]==kPOC_E)||(label_info[result[i]][0]==kPOC_S)){//分词位置
            ts.push_back(WordWithTag(separator));
            for(int j=offset;j<i+1;j++){
                ts.back().word.push_back(sequence[j]);
            }
            offset=i+1;
            if(*(label_info[result[i]]+1)){//输出标签（如果有的话）
                ts.back().tag=label_info[result[i]]+1;
                //printf("%s",label_info[result[i]]+1);
            }
            //if((i+1)<len)putchar(' ');//在分词位置输出空格
        }
    }
};

int TaggingDecoder::segment(RawSentence&raw,
        POCGraph& graph,
        POCGraph& new_graph){
    if(raw.size()==0)return 0;
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    for(int i=0;i<len;i++){
        int pocs=graph[i];
        if(pocs){
            allowed_label_lists[i]=pocs_to_tags[pocs];
        }else{
            allowed_label_lists[i]=pocs_to_tags[15];
        }
    }
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里
    cal_betas();
    find_good_choice();
    new_graph.clear();
    for(int i=0;i<len;i++){
        new_graph.push_back(0);
        for(int j=0;j<model->l_size;j++){
            if(is_good_choice[i*model->l_size+j])
                //new_graph.back()|=(1<<j);
                new_graph.back()|=(1<<(*label_info[j]-'0'));
        }
    }

    for(int i=0;i<len;i++){
        allowed_label_lists[i]=NULL;
    }
}

int TaggingDecoder::segment(POCGraph& poc_graph,RawSentence& raw){
    if(raw.size()==0)return 0;
    for(int i=0;i<(int)raw.size();i++){
        int pocs=poc_graph[i];
        if(pocs){
            allowed_label_lists[i]=pocs_to_tags[pocs];
        }else{
            allowed_label_lists[i]=pocs_to_tags[15];
        }
    }
    //std::cout<<"\n";
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=NULL;
    }
};

int TaggingDecoder::segment(int** tags,RawSentence& raw){
    if(raw.size()==0)return 0;

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=tags[i];
    }
    //std::cout<<"\n";
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=NULL;
    }
};

int TaggingDecoder::segment(int** tags, RawSentence& raw, TaggedSentence& ts){
    if(raw.size()==0)return 0;

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=tags[i];
    }
    //std::cout<<"\n";
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=NULL;
    }

    int c;
    int offset=0;
    ts.clear();
    for(int i=0;i<len;i++){
        if((i==len-1)||(label_info[result[i]][0]==kPOC_E)||(label_info[result[i]][0]==kPOC_S)){//分词位置
            ts.push_back(WordWithTag(separator));
            for(int j=offset;j<i+1;j++){
                ts.back().word.push_back(sequence[j]);
            }
            offset=i+1;
            if(*(label_info[result[i]]+1)){//输出标签（如果有的话）
                ts.back().tag=label_info[result[i]]+1;
                //printf("%s",label_info[result[i]]+1);
            }
            //if((i+1)<len)putchar(' ');//在分词位置输出空格
        }
    }
};

int TaggingDecoder::segment(RawSentence& raw,POCGraph& poc_graph,Lattice& lattice){
    if(raw.size()==0)return 0;
    for(int i=0;i<(int)raw.size();i++){
        int pocs=poc_graph[i];
        //std::cout<<pocs<<" ";
        if(pocs){
            allowed_label_lists[i]=pocs_to_tags[pocs];
        }else{
            allowed_label_lists[i]=pocs_to_tags[15];
        }
    }
    //std::cout<<"\n";
    for(int i=0;i<(int)raw.size();i++){
        sequence[i]=raw[i];
    }
    len=(int)raw.size();
    put_values();//检索出特征值并初始化放在values数组里
    dp();//动态规划搜索最优解放在result数组里
    cal_betas();

    generate_lattice(lattice);

    for(int i=0;i<(int)raw.size();i++){
        allowed_label_lists[i]=NULL;
    }
};

}

