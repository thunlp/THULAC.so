#pragma once
#include "wb_lattice.h"
#include "dat.h"

namespace thulac{
using namespace hypergraph;

class THULACLatticeFeature: public Feature<int,LatticeEdge>{
public:
    typedef void(THULACLatticeFeature::*p_feature_call_back)(Raw&,int&);
    std::string filename;
public:
    THULACLatticeFeature(){
    };
    ~THULACLatticeFeature(){
    };
    int load(){
    }
    inline void extract_features(
            Graph& graph
            ,p_feature_call_back call_back
            ,int filter=0
            ){
        RawSentence key;
        for(int i=0;i<graph.nodes.size();i++){
            Graph::Node& node=graph.nodes[i];
            LatticeEdge& lattice_edge=node.data;
            node.weight=0;

            node.weight=-node.data.margin;
        }
    };
    void add_weights(Graph& graph){
        extract_features(graph,NULL);
    };

};

void lattice_to_sentence(Lattice& lattice,TaggedSentence& sentence, char separator){
    sentence.clear();
    for(int i=0;i<lattice.size();i++){
        sentence.push_back(WordWithTag(separator));
        sentence.back().word=lattice[i].word;
        sentence.back().tag=lattice[i].tag;
    }
};

};//end of thulac

