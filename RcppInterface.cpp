#include "RcppInterface.h"

#include "Utils.cpp"

using namespace boost::interprocess;
using namespace std;
using namespace arma;
using namespace Rcpp;


DebugCollector dbg;
GNGGraphInfo * ggi;
int GNGExample::N=3; 
RcppExport SEXP GNGClient__new(){
   //GNGExample::N=3; g
   
    
    
   managed_shared_memory *  mshm1= new managed_shared_memory(open_only,"SHMemoryPool_Segment1");
   managed_shared_memory *  mshm2= new managed_shared_memory(open_only,"SHMemoryPool_Segment2");
   
   MyMutex * grow_mutex = (mshm1->find< MyMutex >("grow_mutex")).first;		
   MyMutex * database_mutex = (mshm2->find< MyMutex >("database_mutex")).first;	
   SHGNGExampleDatabase * database_vec = (mshm2->find< SHGNGExampleDatabase >("database_vec")).first;	
   
   ScopedLock sc(*grow_mutex); 
   
  // grow_mutex->lock();

   GNGGraphInfo * ggi2 = (mshm1->find<GNGGraphInfo >("SHGraphInfo")).first; //musi byc stworzony : kolejne miejsce na blad !
   
   
   GNGClient * gngClient = new GNGClient();

   
   gngClient->ggi = ggi2;
   gngClient->g_database = new GNGDatabaseSimple(database_mutex, database_vec);
   

   gngClient->grow_mutex = grow_mutex;

   Rcpp::XPtr<GNGClient> ptr(gngClient,true);


   
   //grow_mutex->unlock();

   return wrap(ptr); 
}

//czyta 3 node

RcppExport SEXP GNGClient__updateBuffer(SEXP _xp){
    
    
    Rcpp::XPtr<GNGClient> ptr(_xp);

    ScopedLock sc(*ptr->grow_mutex);
    
    GraphAccess & graph = *(ptr->readGraph());
    
    //copy routine !
    
    ptr->buffer.clear();
    ptr->buffer.resize(graph.getNumberNodes());
    
    
    
    for(int i=0;i<graph.getNumberNodes();++i){
        ptr->buffer[i] = (graph.getPool())[i]; //operator= !! wazne zeby dawac tego typu rzeczy do klas
        if(ptr->buffer[i].edgesCount==0) continue;
        
        //replace
        //TO-DO: hack
        /*
        FOREACH(edg,ptr->buffer[i].edges){
            if(edg->nr>((int)ptr->buffer.size()-1)) edg=ptr->buffer[i].edges.erase(edg);
           
        }
        */
    }

    
    return wrap(0);    
}

RcppExport SEXP GNGClient__getNumberNodesOnline(SEXP _xp){
    
    
    
    Rcpp::XPtr<GNGClient> ptr(_xp);
    ScopedLock sc(*ptr->grow_mutex);
    
    GraphAccess & graph = *(ptr->readGraph());
    return wrap((int)(graph.getNumberNodes()));
}

RcppExport SEXP GNGClient__getNumberNodes(SEXP _xp){
    
    Rcpp::XPtr<GNGClient> ptr(_xp);
    
    return wrap((int)(ptr->buffer.size()));
}

RcppExport SEXP GNGClient__getNodeNumberEdges(SEXP _xp, SEXP _nr){
    Rcpp::XPtr<GNGClient> ptr(_xp);
    return wrap((int)
            (ptr->buffer[as<int>(_nr)].edges.size())
    );
}

RcppExport SEXP GNGClient__getNodeMatrix(SEXP _xp){
    Rcpp::XPtr<GNGClient> ptr(_xp); 
    
    int nodes = (int)ptr->buffer.size();
    
    Rcpp::NumericMatrix node_matrix(nodes,GNGExample::N);
    
    for(int i=0;i<nodes;++i){
        for(int j=0;j<GNGExample::N;++j){
            node_matrix(i,j) = ptr->buffer[i].position[j];
        }
    }
    
    return wrap(node_matrix);
}


RcppExport SEXP GNGClient__getNode(SEXP _xp, SEXP _nr){

    Rcpp::XPtr<GNGClient> ptr(_xp);
    int nr = as<int>(_nr);

    int a = 1;
 
   
    GNGNodeOffline * requested_node = &ptr->buffer[nr];
 
    int edg = requested_node->edgesCount;
    
    //cout<<edg<<endl;
    
    Rcpp::NumericVector node(edg+3);
    
    
    //memcpy?, arma position ?
    
    int i;
    for(i=0;i<3;++i) {
        node[i]=requested_node->position[i];
    }

    i = 2;
    
    if(edg>0){
        FOREACH(it,requested_node->edges){
            
           ++i;
           node[i] = it->nr;
          if(i==edg+2) break; //added  new edge meanwhile
        }  
    }    

    
    return wrap(node);
}



RcppExport SEXP GNGClient__addExamples(SEXP _xp, SEXP _examples){
    Rcpp::XPtr<GNGClient> ptr(_xp);
    Rcpp::NumericMatrix examples(_examples);
    cout<<examples.nrow()<<","<<examples.ncol()<<endl;

    ptr->grow_mutex->lock();
    double * pos = new double[3];
    for(int i=0;i<examples.nrow();++i){
        for(int j=0;j<3;++j) pos[j] = (double)(examples(i,j));
        GNGExample ex(&pos[0]);
        ptr->g_database->addExample(&ex);
    }

    
    ptr->grow_mutex->unlock();   
    
    return wrap(1);
}