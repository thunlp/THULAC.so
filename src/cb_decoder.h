#ifndef __DP_H__
#define __DP_H__
#include<cstdlib>

namespace permm{


/**
 * topological information about a node
 * type的定义： 默认0，如果是开始节点+1，如果是结尾节点+2
 * */
struct Node{
    int type;///默认0，如果是开始节点+1，如果是结尾节点+2
    int* predecessors;///ends with a -1
    int* successors;///ends with a -1
};

///**given prececessors, calculate successors*/
//int* dp_cal_successors(int node_count,Node* nodes);

//a structure for alphas and betas
struct Alpha_Beta{
    int value;
    int node_id;
    int label_id;
};

inline int alpha_beta_comp(const void* a,const void* b){
    return ((Alpha_Beta*)b)->value-((Alpha_Beta*)a)->value;
};

/*
the n-best heap for the n-best searching
This is a min-heap
*/
inline void nb_heap_insert(Alpha_Beta* heap,int max_size,int& count,Alpha_Beta& element){
    if(count==max_size){//heap is full
        if(element.value<heap[0].value)return;//do not insert
        //remove the top and insert it
        //top_down;
        int ind=0;
        while(1){
            if((ind*2+1)<count){//has children
                if(heap[ind*2+1].value<element.value){//left is smaller
                    if(((ind*2+2)<count)&&(heap[ind*2+2].value<heap[ind*2+1].value)){
                        heap[ind]=heap[ind*2+2];
                        ind=ind*2+2;
                    }else{
                        heap[ind]=heap[ind*2+1];
                        ind=ind*2+1;
                    }
                }else{
                    if(((ind*2+2)<count)&&(heap[ind*2+2].value<element.value)){
                        heap[ind]=heap[ind*2+2];
                        ind=ind*2+2;
                    }else{
                        break;
                    }
                }
            }else{
                break;
            }
        }
        heap[ind]=element;
    }else{
        int ind=count++;
        //buttom up
        while(1){
            if(ind==0)break;
            if(heap[(ind-1)/2].value<=element.value)break;
            heap[ind]=heap[(ind-1)/2];
            ind=(ind-1)/2;
        }
        heap[ind]=element;
    }
};


/** The DP algorithm(s) for path labeling */
inline int dp_decode(
        int l_size,///标签个数
        int* ll_weights,///标签间权重
        int node_count,///节点个数
        Node* nodes,///节点数据
        int* values,///value for i-th node with j-th label
        Alpha_Beta* alphas,///alpha value (and the pointer) for i-th node with j-th label
        int* result,
        int** pre_labels=NULL,///每种标签可能的前导标签（以-1结尾）
        int** allowed_label_lists=NULL///每个节点可能的标签列表
        ){
    //calculate alphas
    int node_id;
    int* p_node_id;
    int* p_pre_label;
    int* p_allowed_label;//指向当前字所有可能标签的数组的指针
    register int k;//当前字的前一个节点可能的标签（的编号）
    register int j;//当前字某一个可能的标签（的编号）
    register Alpha_Beta* tmp;
    Alpha_Beta best;best.node_id=-1;
    Alpha_Beta* pre_alpha;
    int score;
    
    for(int i=0;i<node_count*l_size;i++)alphas[i].node_id=-2;
    for(int i=0;i<node_count;i++){//for each node
        p_allowed_label=allowed_label_lists?allowed_label_lists[i]:NULL;
        j=-1;
        int max_value=0;
        int has_max_value=0;
        while((p_allowed_label?
                    ((j=(*(p_allowed_label++)))!=-1)://如果有指定，则按照列表来
                    ((++j)!=l_size))){//否则枚举
            if((!has_max_value) || (max_value<values[i*l_size+j])){
                has_max_value=1;
                max_value=values[i*l_size+j];
            }
        }
        p_allowed_label=allowed_label_lists?allowed_label_lists[i]:NULL;
        j=-1;
        while((p_allowed_label?
                    ((j=(*(p_allowed_label++)))!=-1)://如果有指定，则按照列表来
                    ((++j)!=l_size))){//否则枚举
            //if(max_value-20000>values[i*l_size+j])continue;//
            tmp=&alphas[i*l_size+j];
            tmp->value=0;
            p_node_id=nodes[i].predecessors;
            p_pre_label=pre_labels?pre_labels[j]:NULL;
            while((node_id=*(p_node_id++))>=0){//枚举前继节点
                k=-1;
                while(p_pre_label?
                        ((k=(*p_pre_label++))!=-1):
                        ((++k)!=l_size)
                        ){
                    pre_alpha=alphas+node_id*l_size+k;
                    if(pre_alpha->node_id==-2)continue;//not reachable
                    score=pre_alpha->value+ll_weights[k*l_size+j];
                    if((tmp->node_id<0)||(score>tmp->value)){
                        tmp->value=score;
                        tmp->node_id=node_id;
                        tmp->label_id=k;
                    }
                }
            }
            tmp->value+=values[i*l_size+j];
            
            if((nodes[i].type==1)||(nodes[i].type==3))
                tmp->node_id=-1;
            if(nodes[i].type>=0){
                if((best.node_id==-1)||(best.value<tmp->value)){
                    best.value=tmp->value;
                    best.node_id=i;
                    best.label_id=j;
                }
            }
        }
        //std::cout<<i<<" "<<best.value<<"\n";
    }
    //find the path and label the nodes of it.
    tmp=&best;
    while(tmp->node_id>=0){
        result[tmp->node_id]=tmp->label_id;
        tmp=&(alphas[(tmp->node_id)*l_size+(tmp->label_id)]);
    }
    //debug
    /*(for(int i=0;i<node_count;i++){//for each node
        p_allowed_label=allowed_label_lists?allowed_label_lists[i]:NULL;   
        j=-1;
        std::cerr<<values[i*l_size+result[i]]<<" ";
        while((p_allowed_label?
                    ((j=(*(p_allowed_label++)))!=-1)://如果有指定，则按照列表来
                    ((++j)!=l_size))){//否则枚举
            tmp=&alphas[i*l_size+j];
            std::cerr<<values[i*l_size+j]<<" ";  
        }
        std::cerr<<"\n";
    }
    std::cerr<<"\n";*/
    //end of debug
    return best.value;
};


inline void dp_cal_betas(
        /**something about the model*/
        int l_size,///标签个数
        int* ll_weights,///标签间权重
        /**something about the graph*/
        int node_count,//numbers of nodes
        Node* nodes,///节点数据
        //something about the scores
        int* values,//value for i-th node with j-th label
        Alpha_Beta* betas,//alpha value (and the pointer) for i-th node with j-th label
        int** post_labels=NULL,///每种标签可能的前导标签（以-1结尾）
        int** allowed_label_lists=NULL///每个节点可能的标签列表
        ){
    int node_id;
    int* p_node_id;
    int* p_post_label;
    int* p_allowed_label;
    int k;
    int j;
    
    Alpha_Beta* tmp;
    Alpha_Beta* post_beta;
    int score=0;
    
    for(int i=0;i<node_count*l_size;i++)betas[i].node_id=-2;
    
    for(int i=node_count-1;i>=0;i--){//for each node
        p_allowed_label=allowed_label_lists?allowed_label_lists[i]:NULL;
        j=-1;
        while((p_allowed_label?
                    ((j=(*(p_allowed_label++)))!=-1)://如果有指定，则按照列表来
                    ((++j)!=l_size))){//否则枚举
        //for(int j=0;j<l_size;j++){//for each label
            tmp=&betas[i*l_size+j];
            tmp->value=0;
            p_node_id=nodes[i].successors;
            p_post_label=post_labels?post_labels[j]:NULL;
            while((node_id=*(p_node_id++))>=0){
                k=-1;
                while(p_post_label?
                        ((k=(*p_post_label++))!=-1):
                        ((++k)!=l_size)
                        ){
                    post_beta=betas+node_id*l_size+k;
                    if(post_beta->node_id==-2)continue;//not reachable
                    score=post_beta->value+ll_weights[j*l_size+k];
                    if((tmp->node_id<0)||(score>tmp->value)){
                        tmp->value=score;
                        tmp->node_id=node_id;
                        tmp->label_id=k;
                    }
                }
            }
            tmp->value+=values[i*l_size+j];
            if((nodes[i].type==2)||(nodes[i].type==3))
                tmp->node_id=-1;
        }
    }
    return;
    
};


inline void dp_nb_decode(
        /**something about the model*/
        int l_size,///标签个数
        int* ll_weights,///标签间权重
        /**something about the graph*/
        int node_count,///节点个数
        Node* nodes,///节点数据
        //something about the scores
        int* values,//value for i-th node with j-th label
        int nb,//n-best
        Alpha_Beta* alphas,//alpha value (and the pointer) for i-th node with j-th label
        //something about the result
        int* result
        ){
    
    //calculate alphas
    int node_id;
    int* p_node_id;
    Alpha_Beta best[nb];
    int best_count=0;
    int score;
    int count;
    for(int i=0;i<node_count;i++){//for each node
        for(int j=0;j<l_size;j++){//for each label
            p_node_id=nodes[i].predecessors;
            count=0;
            for(int n=0;n<nb;n++){
                alphas[(i*l_size+j)*nb+n].node_id=-2;
                alphas[(i*l_size+j)*nb+n].value=0;
            }
            while((node_id=*(p_node_id++))>=0){
                for(int k=0;k<l_size;k++){
                    score=values[i*l_size+j]+ll_weights[k*l_size+j];
                    for(int n=0;n<nb;n++){
                        if(alphas[(node_id*l_size+k)*nb+n].node_id==-2)continue;
                        Alpha_Beta tmp={score+alphas[(node_id*l_size+k)*nb+n].value,
                                node_id,k*nb+n};
                        //printf("add, %d %d %d %d %d\n",i,j,node_id,k,n);
                        nb_heap_insert(alphas+(i*l_size+j)*nb,nb,count,tmp);
                    }
                }
            }
            
            if((nodes[i].type==1)||(nodes[i].type==3)){
                Alpha_Beta tmp={values[i*l_size+j],-1,0};
                nb_heap_insert(alphas+(i*l_size+j)*nb,nb,count,tmp);
            }
            /*
            printf("(position=%d, label=%d)\n",i,j);
            for(int n=0;n<count;n++){
                printf("value=%d pre_position=%d pre_label=%d\n",alphas[(i*l_size+j)*nb+n].value,
                       alphas[(i*l_size+j)*nb+n].node_id,
                       alphas[(i*l_size+j)*nb+n].label_id/nb);
            }
            getchar();*/
            if(nodes[i].type>=2){
                for(int n=0;n<nb;n++){
                    Alpha_Beta tmp={alphas[(i*l_size+j)*nb+n].value,
                                i,j*nb+n};
                    nb_heap_insert(best,nb,best_count,tmp);
                }
            }
        }
    }
    qsort(best,nb,sizeof(Alpha_Beta),alpha_beta_comp);
    //find the path and label the nodes of it.
    
    //for(node_id=0;node_id<node_count;node_id++)
    //    result[node_id]=-1;
    
    for(int n=0;n<nb;n++){
        Alpha_Beta* tmp=&best[n];
        while(tmp->node_id>=0){
            result[n*node_count+tmp->node_id]=(tmp->label_id)/nb;
            tmp=&(alphas[(tmp->node_id)*l_size*nb+(tmp->label_id)]);
        }
    }
    return;
};

/**given prececessors, calculate successors*/
inline int* dp_cal_successors(int node_count,Node* nodes){
    int size=0;
    int* out_degrees=(int*)calloc(sizeof(int),node_count);
    int* p_node_id;
    int node_id;
    for(int i=0;i<node_count;i++){
        p_node_id=nodes[i].predecessors;
        while((node_id=*(p_node_id++))>=0){
            out_degrees[node_id]++;
        }
    }
    for(int i=0;i<node_count;i++){
        node_id=out_degrees[i]+1;
        out_degrees[i]=size;
        size+=node_id;
    }
    int*buffer=(int*)malloc(sizeof(int)*size);
    for(int i=0;i<size;i++)buffer[i]=-1;
    
    //for(int i=0;i<size;i++)printf("%d ",buffer[i]);
    //printf("\n");
    
    for(int i=0;i<node_count;i++){
        nodes[i].successors=buffer+out_degrees[i];
        out_degrees[i]=0;
    }
    for(int i=0;i<node_count;i++){
        p_node_id=nodes[i].predecessors;
        while((node_id=*(p_node_id++))>=0){
            nodes[node_id].successors[out_degrees[node_id]++]=i;
        }
    }
    //for(int i=0;i<size;i++)printf("%d ",buffer[i]);
    //printf("\n");
    //getchar();
    free(out_degrees);
    return buffer;
};


}
#endif
