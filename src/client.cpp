/**
 * @file client.cpp
 * @brief Methods in client class
 */

#include "client.h"
#include "message_wrap.h"

client::client(thread_cfg* _cfg,string_server* _str_server):cfg(_cfg)
                            ,str_server(_str_server),parser(_str_server){

}

void client::GetId(request_or_reply& req){
    req.parent_id=cfg->get_inc_id();
}

void client::Send(request_or_reply& req){
    if(req.parent_id==-1){
        GetId(req);
    }

    // C-SPARQL should be handled in stream_query_client
    assert(!req.stream_info.is_stream_query);
    // normal SPARQL
    int tid_base = cfg->client_num + 2 +
                               global_stream_multithread_factor * 
                               global_num_stream_client;

    if(req.use_index_vertex() && global_enable_index_partition){
        int nthread=max(1,min(global_multithread_factor,global_num_server));
        for(int i=0;i<cfg->m_num;i++){
            for(int j=0;j<nthread;j++){
                req.mt_total_thread=nthread;
                req.mt_current_thread=j;
                SendR(cfg,i,tid_base + j,req);
            }
        }
        return ;
    }

    req.first_target=mymath::hash_mod(req.cmd_chains[0],cfg->m_num);
    // one-to-one mapping
    //int server_per_client=  cfg->server_num  / cfg->client_num;
    //int mid=cfg->client_num + server_per_client*cfg->t_id + cfg->get_random() % server_per_client;

    // random mapping
    //int tid=cfg->client_num + cfg->get_random() % cfg->server_num;
    SendR(cfg,req.first_target,tid_base,req);
}

request_or_reply client::Recv(){
    request_or_reply r = RecvR(cfg);
    assert(!r.stream_info.is_stream_query);

    if(r.use_index_vertex() && global_enable_index_partition ){
        int nthread=max(1,min(global_multithread_factor,global_num_server));
        for(int count=0;count<cfg->m_num * nthread-1 ;count++){
            request_or_reply r2=RecvR(cfg);
            r.silent_row_num +=r2.silent_row_num;
            int new_size=r.result_table.size()+r2.result_table.size();
            r.result_table.reserve(new_size);
            r.result_table.insert( r.result_table.end(), r2.result_table.begin(), r2.result_table.end());
        }
    }
    return r;
}

void client::print_result(request_or_reply& reply,int row_to_print){
    for(int i=0;i<row_to_print;i++){
        cout<<i+1<<":  ";
        for(int c=0;c<reply.column_num();c++){
            int id=reply.get_row_column(i,c);
            if(str_server->id2str.find(id)==str_server->id2str.end()){
                cout<<"NULL  ";
            } else {
                cout<<str_server->id2str[id]<< ":" << id << "  ";
            }
        }
        cout<<endl;
    }
};
