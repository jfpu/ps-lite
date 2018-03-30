#include <iostream>
#include "ps/ps.h"

using namespace std;
using namespace ps;
template <class Val>

class KVServerDefaultHandle1 {      //functor，用与处理server收到的来自worker的请求
public:
    // req_meta是存储该请求的一些元信息，即请求来自于哪个节点，发送给哪个节点等等
    // req_data即发送过来的数据
    // server即指向当前server对象的指针
    void operator() (const KVMeta& req_meta, const KVPairs<Val>& req_data, KVServer<Val>* server) {
        size_t n = req_data.keys.size();
        KVPairs<Val> res;
        if (req_meta.push) { //收到的是push请求
            CHECK_EQ(n, req_data.vals.size());
        } else {            //收到的是pull请求
            res.keys = req_data.keys;
            res.vals.resize(n);
        }
        for (size_t i = 0;i < n; ++i) {
            Key key = req_data.keys[i];
            if (req_meta.push) {    //push请求
                store[key] += req_data.vals[i]; //此处的操作是将相同key的value相加
            } else {                    //pull请求
                res.vals[i] = store[key];
            }
        }
        server->Response(req_meta, res);
    }
private:
    std::unordered_map<Key, Val> store;
};

void StartServer() {
    if (!IsServer()) return;
    cout << "num of workers[" << NumWorkers() << "]" << endl;
    cout << "num of servers[" << NumServers() << "]" << endl;
    auto server = new KVServer<float>(0);
    server->set_request_handle(KVServerDefaultHandle1<float>());   //注册functor
    RegisterExitCallback([server](){ delete server; });
}

void RunWorker() {
    if (!IsWorker()) return;
    cout << "start Worker rank = " << MyRank() << endl;
    KVWorker<float> kv(0, 0);
    // init
    int num = 10;
    vector<Key> keys(num);
    vector<float> vals(num);
    int rank = MyRank();
    for (int i = 0;i < num; ++i) {
        keys[i] = i;
        vals[i] = i+10;
    }
    // push
    int repeat = 1;
    vector<int> ts;
    for (int i = 0;i < repeat; ++i) {
        ts.push_back(kv.Push(keys, vals));  //kv.Push()返回的是该请求的timestamp
    }
    for (int t : ts) kv.Wait(t);
    // pull
    std::vector<float> rets;
    kv.Wait(kv.Pull(keys, &rets));
    for (size_t i = 0;i < rets.size(); ++i) {
        cout << MyRank() << " rets[" << i << "]: " << rets[i] << endl;
    }
    cout << endl;
}

int main(int argc, char* argv[]) {
    StartServer();
    Start(0);    //启动,Postoffice::start()
    RunWorker();
    Finalize(0, true); //结束。每个节点都需要执行这个函数。
    return 0;
}
