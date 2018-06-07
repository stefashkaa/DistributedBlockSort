/*$TET$actor*/

/*
	Distributed block sort: a sample application for data processing in mobile ad hoc networks
*/

#include <time.h>
#include <iostream>

#define PARALLEL_EXECUTION
#define USE_OPENMP

#include "templet.hpp"

#include <omp.h>
#include <algorithm>
#include <list>
#include <string>

#include <stdlib.h>

#include "Everest.cpp"

using namespace std;

const string EVEREST_URL = "https://everest.distcomp.org";
const string LABEL = "blocksort";
const string USERNAME = "stefanpopov";
const string PASSWORD = "qwaszx1";
const string SORTER_ID = "5afc82b3140000203c6f7b75";
const string MERGER_ID = "5afc83fc140000203c6f7b7d";
const string IS_SORTED_ID = "5b150d0514000026876fcdc7";

// 1
//const int NUM_BLOCKS = 2;
//const int BLOCK_SIZE = 16000000;
// 2
const int NUM_BLOCKS = 4;
const int BLOCK_SIZE = 250;//8000000;
// 3
//const int NUM_BLOCKS = 8;
//const int BLOCK_SIZE = 4000000;
// 4
//const int NUM_BLOCKS = 16;
//const int BLOCK_SIZE = 2000000;
// 5
//const int NUM_BLOCKS = 32;
//const int BLOCK_SIZE = 1000000;
// 6
//const int NUM_BLOCKS = 64;
//const int BLOCK_SIZE = 500000;
// 7
//const int NUM_BLOCKS = 128;
//const int BLOCK_SIZE = 250000;

vector<Everest::File*> fileBlocks;
vector<Everest::Job*> allJobs;

struct task_sort;
struct task_merge;

struct everest_queue {
	enum { SORT, MERGE } type;
	task_sort*  sort;
	task_merge* merge;
	bool done;
};

/*$TET$*/

using namespace TEMPLET;

struct my_engine : engine{
	my_engine(int argc, char *argv[]){
		TEMPLET::init(this, argc, argv);
	}
	void run(){ TEMPLET::run(this); }
	void map(){ TEMPLET::map(this); }
};

#pragma templet ~mes=

struct mes : message{
	mes(actor*a, engine*e, int t) : _where(CLI), _cli(a), _client_id(t){
		TEMPLET::init(this, a, e);
	}

	void send(){
		if (_where == CLI){ TEMPLET::send(this, _srv, _server_id); _where = SRV; }
		else if (_where == SRV){ TEMPLET::send(this, _cli, _client_id); _where = CLI; }
	}

/*$TET$mes$$data*/
	int i;
/*$TET$*/

	enum { CLI, SRV } _where;
	actor* _srv;
	actor* _cli;
	int _client_id;
	int _server_id;
};

#pragma templet ~task_sort=

struct task_sort : message{
	task_sort(actor*a, engine*e, int t) : _where(CLI), _cli(a), _client_id(t){
		TEMPLET::init(this, a, e);
	}

	void send(){
		if (_where == CLI){ TEMPLET::send(this, _srv, _server_id); _where = SRV; }
		else if (_where == SRV){ TEMPLET::send(this, _cli, _client_id); _where = CLI; }
	}

/*$TET$task_sort$$data*/
	int i; // index of block to be sorted
/*$TET$*/

	enum { CLI, SRV } _where;
	actor* _srv;
	actor* _cli;
	int _client_id;
	int _server_id;
};

#pragma templet ~task_merge=

struct task_merge : message{
	task_merge(actor*a, engine*e, int t) : _where(CLI), _cli(a), _client_id(t){
		TEMPLET::init(this, a, e);
	}

	void send(){
		if (_where == CLI){ TEMPLET::send(this, _srv, _server_id); _where = SRV; }
		else if (_where == SRV){ TEMPLET::send(this, _cli, _client_id); _where = CLI; }
	}

/*$TET$task_merge$$data*/
	int i, j; // indices of blocks to be merged
/*$TET$*/

	enum { CLI, SRV } _where;
	actor* _srv;
	actor* _cli;
	int _client_id;
	int _server_id;
};

#pragma templet *everest(s?task_sort,m?task_merge,timer?mes)

struct everest : actor{
	enum tag{START,TAG_s,TAG_m,TAG_timer};

	everest(my_engine&e){
		TEMPLET::init(this, &e, everest_recv_adapter);
/*$TET$everest$everest*/
		everestAPI = new Everest(USERNAME, PASSWORD, LABEL);
		createFiles();
/*$TET$*/
	}

	bool access(message*m){ return TEMPLET::access(m, this); }
	bool access(message&m){ return TEMPLET::access(&m, this); }

	void at(int _at){ TEMPLET::at(this, _at); }
	void delay(double t){ TEMPLET::delay(this, t); }
	double time(){ return TEMPLET::time(this); }
	void stop(){ TEMPLET::stop(this); }

	void s(task_sort&m){m._server_id=TAG_s; m._srv=this;}
	void m(task_merge&m){m._server_id=TAG_m; m._srv=this;}
	void timer(mes&m){m._server_id=TAG_timer; m._srv=this;}

	static void everest_recv_adapter (actor*a, message*m, int tag){
		switch(tag){
			case TAG_s: ((everest*)a)->s_handler(*((task_sort*)m)); break;
			case TAG_m: ((everest*)a)->m_handler(*((task_merge*)m)); break;
			case TAG_timer: ((everest*)a)->timer_handler(*((mes*)m)); break;
		}
	}

	void s_handler(task_sort&m){
/*$TET$everest$s*/
		tsort.push_back(&m);
/*$TET$*/
	}

	void m_handler(task_merge&m){
/*$TET$everest$m*/
		tmerge.push_back(&m);
/*$TET$*/
	}

	void timer_handler(mes&m){
/*$TET$everest$timer*/

		// checking OMP task execution
		for (std::list<everest_queue*>::iterator it = queue.begin(); it != queue.end();)
		{
			everest_queue* eq = *it;
			if (eq->done) {
				switch (eq->type) {
					case everest_queue::SORT: eq->sort->send(); break;
					case everest_queue::MERGE: eq->merge->send(); break;
					default: cout << "unknown task type" << endl;
				}
				delete eq;
				it = queue.erase(it);
			}
			else it++;
		}

		// sending OMP tasks to the queue
		while (!tsort.empty()) {
			task_sort* t = tsort.front();
			tsort.pop_front();

			everest_queue* eq = new everest_queue;

			eq->type = everest_queue::SORT;
			eq->sort = t;
			eq->done = false;
			queue.push_back(eq);

#pragma omp task firstprivate(eq)
			{
				auto file = fileBlocks.at(eq->sort->i);
				json inputs;
				inputs["file"] = file->uri;

				auto job = everestAPI->runJob(SORTER_ID, "sorter", inputs);

				while(job->state != Everest::State::DONE) {
					job->refresh();
					if (job->state == Everest::State::FAILED) {
						cout << "The last job was failed!" << endl;
						exit(1);
					}
				}
				//save result
				file->uri = job->result["outSort"];
				fileBlocks.at(eq->sort->i) = file;

				allJobs.push_back(job);
				eq->done = true;
			}
		}

		while (!tmerge.empty()) {
			task_merge* t = tmerge.front();
			tmerge.pop_front();

			everest_queue* eq = new everest_queue;

			eq->type = everest_queue::MERGE;
			eq->merge = t;
			eq->done = false;
			queue.push_back(eq);

#pragma omp task firstprivate(eq)
			{
				auto iFile = fileBlocks.at(eq->merge->i);
				auto jFile = fileBlocks.at(eq->merge->j);
				json inputs;

				inputs["file1"] = iFile->uri;
				inputs["file2"] = jFile->uri;
				inputs["i"] = to_string(eq->merge->i);
				inputs["j"] = to_string(eq->merge->j);

				auto job = everestAPI->runJob(MERGER_ID, "merger", inputs);

				while(job->state != Everest::State::DONE) {
					job->refresh();
					if (job->state == Everest::State::FAILED) {
						cout << "The last job was failed!" << endl;
						exit(1);
					}
				}
				//save result
				iFile->uri = job->result["outMerge1"];
				jFile->uri = job->result["outMerge2"];

				fileBlocks.at(eq->merge->i) = iFile;
				fileBlocks.at(eq->merge->j) = jFile;

				allJobs.push_back(job);
				eq->done = true;
			}
		}
		m.send();
/*$TET$*/
	}

/*$TET$everest$$code&data*/
	~everest() {
		isSorted();
		printResult();
		//will be removed all files in temp directory
		everestAPI->deleteAllFiles();
		//will be removed all jobs
		for(Everest::Job* j : allJobs) {
			j->remove();
		}
		everestAPI->removeAccessToken();
		cout << endl << "Everest clean-up" << endl;
		vector<Everest::File*>().swap(fileBlocks);
	}

	Everest *everestAPI;
	std::list<task_sort*>  tsort;
	std::list<task_merge*> tmerge;
	std::list<everest_queue*> queue;

	void createFiles() {
		string name = "./build/file";
		srand(1);

		for(int block = 0; block < NUM_BLOCKS; block++) {
			string fullName = name + to_string(block);

			FILE* f = fopen(fullName.c_str(), "wb");
			fwrite(&BLOCK_SIZE, sizeof(int), 1, f);

			for(int index = 0; index < BLOCK_SIZE; index++) {
				int r = rand();
				fwrite(&r, sizeof(int), 1, f);
			}
			fclose(f);

			fileBlocks.push_back(everestAPI->uploadFile(fullName));
			remove(fullName.c_str());
		}
	}

	void isSorted() {
		json files;
		for(int i = 0; i < (int)fileBlocks.size(); i++) {
			auto f = fileBlocks.at(i);
			files[i] = f->uri;
		}
		json inputs;
		inputs["files"] = files;

		auto job = everestAPI->runJob(IS_SORTED_ID, "isSorted", inputs);

		while(job->state != Everest::State::DONE) {
			job->refresh();
			if (job->state == Everest::State::FAILED) {
				cout << "The last job was failed!" << endl;
				exit(1);
			}
		}
		auto answer = everestAPI->downloadFile(job->result["answer"]);
		cout << endl << answer << endl;
	}

	void printResult() {
		cout << endl << "Array blocks available at following links:" << endl;
		for(int i = 0; i < (int)fileBlocks.size(); i++) {
			auto file = fileBlocks.at(i);
			cout << EVEREST_URL << file->uri << endl;
		}
	}
/*$TET$*/
};

#pragma templet *timer(p!mes)+

struct timer : actor{
	enum tag{START,TAG_p};

	timer(my_engine&e):p(this, &e, TAG_p){
		TEMPLET::init(this, &e, timer_recv_adapter);
		TEMPLET::init(&_start, this, &e);
		TEMPLET::send(&_start, this, START);
/*$TET$timer$timer*/
/*$TET$*/
	}

	bool access(message*m){ return TEMPLET::access(m, this); }
	bool access(message&m){ return TEMPLET::access(&m, this); }

	void at(int _at){ TEMPLET::at(this, _at); }
	void delay(double t){ TEMPLET::delay(this, t); }
	double time(){ return TEMPLET::time(this); }
	void stop(){ TEMPLET::stop(this); }

	mes p;

	static void timer_recv_adapter (actor*a, message*m, int tag){
		switch(tag){
			case TAG_p: ((timer*)a)->p_handler(*((mes*)m)); break;
			case START: ((timer*)a)->start(); break;
		}
	}

	void start(){
/*$TET$timer$start*/
		p.send();
/*$TET$*/
	}

	void p_handler(mes&m){
/*$TET$timer$p*/
		int milliseconds = 10;
		struct timespec ts;
		ts.tv_sec = milliseconds / 1000;
		ts.tv_nsec = (milliseconds % 1000) * 1000000;
		nanosleep(&ts, NULL);
		p.send();
/*$TET$*/
	}

/*$TET$timer$$code&data*/
/*$TET$*/
	message _start;
};

#pragma templet *sorter(out!mes,e!task_sort)+

struct sorter : actor{
	enum tag{START,TAG_out,TAG_e};

	sorter(my_engine&e):out(this, &e, TAG_out),e(this, &e, TAG_e){
		TEMPLET::init(this, &e, sorter_recv_adapter);
		TEMPLET::init(&_start, this, &e);
		TEMPLET::send(&_start, this, START);
/*$TET$sorter$sorter*/
/*$TET$*/
	}

	bool access(message*m){ return TEMPLET::access(m, this); }
	bool access(message&m){ return TEMPLET::access(&m, this); }

	void at(int _at){ TEMPLET::at(this, _at); }
	void delay(double t){ TEMPLET::delay(this, t); }
	double time(){ return TEMPLET::time(this); }
	void stop(){ TEMPLET::stop(this); }

	mes out;
	task_sort e;

	static void sorter_recv_adapter (actor*a, message*m, int tag){
		switch(tag){
			case TAG_out: ((sorter*)a)->out_handler(*((mes*)m)); break;
			case TAG_e: ((sorter*)a)->e_handler(*((task_sort*)m)); break;
			case START: ((sorter*)a)->start(); break;
		}
	}

	void start(){
/*$TET$sorter$start*/
		e.i = i; // request to the service for sorting block i
		e.send();
		//block_sort(i);
		//out.send();
/*$TET$*/
	}

	void out_handler(mes&m){
/*$TET$sorter$out*/
/*$TET$*/
	}

	void e_handler(task_sort&m){
/*$TET$sorter$e*/
		out.send();
/*$TET$*/
	}

/*$TET$sorter$$code&data*/
	int i;
/*$TET$*/
	message _start;
};

#pragma templet *producer(in?mes,out!mes)

struct producer : actor{
	enum tag{START,TAG_in,TAG_out};

	producer(my_engine&e):out(this, &e, TAG_out){
		TEMPLET::init(this, &e, producer_recv_adapter);
/*$TET$producer$producer*/
		bc = NUM_BLOCKS;
		i = 0;
/*$TET$*/
	}

	bool access(message*m){ return TEMPLET::access(m, this); }
	bool access(message&m){ return TEMPLET::access(&m, this); }

	void at(int _at){ TEMPLET::at(this, _at); }
	void delay(double t){ TEMPLET::delay(this, t); }
	double time(){ return TEMPLET::time(this); }
	void stop(){ TEMPLET::stop(this); }

	void in(mes&m){m._server_id=TAG_in; m._srv=this;}
	mes out;

	static void producer_recv_adapter (actor*a, message*m, int tag){
		switch(tag){
			case TAG_in: ((producer*)a)->in_handler(*((mes*)m)); break;
			case TAG_out: ((producer*)a)->out_handler(*((mes*)m)); break;
		}
	}

	void in_handler(mes&m){
/*$TET$producer$in*/
		bc--;
		if (!bc) {
			out_handler(m);
		}
/*$TET$*/
	}

	void out_handler(mes&m){
/*$TET$producer$out*/
		if (i == NUM_BLOCKS) return;
		out.i = i++;
		out.send();
/*$TET$*/
	}

/*$TET$producer$$code&data*/
	int i, bc;
/*$TET$*/
};

#pragma templet *merger(in?mes,out!mes,e!task_merge)

struct merger : actor{
	enum tag{START,TAG_in,TAG_out,TAG_e};

	merger(my_engine&e):out(this, &e, TAG_out),e(this, &e, TAG_e){
		TEMPLET::init(this, &e, merger_recv_adapter);
/*$TET$merger$merger*/
		is_first = true;
		_in = 0;
/*$TET$*/
	}

	bool access(message*m){ return TEMPLET::access(m, this); }
	bool access(message&m){ return TEMPLET::access(&m, this); }

	void at(int _at){ TEMPLET::at(this, _at); }
	void delay(double t){ TEMPLET::delay(this, t); }
	double time(){ return TEMPLET::time(this); }
	void stop(){ TEMPLET::stop(this); }

	void in(mes&m){m._server_id=TAG_in; m._srv=this;}
	mes out;
	task_merge e;

	static void merger_recv_adapter (actor*a, message*m, int tag){
		switch(tag){
			case TAG_in: ((merger*)a)->in_handler(*((mes*)m)); break;
			case TAG_out: ((merger*)a)->out_handler(*((mes*)m)); break;
			case TAG_e: ((merger*)a)->e_handler(*((task_merge*)m)); break;
		}
	}

	void in_handler(mes&m){
/*$TET$merger$in*/
		_in = &m;
		merge();
/*$TET$*/
	}

	void out_handler(mes&m){
/*$TET$merger$out*/
		merge();
/*$TET$*/
	}

	void e_handler(task_merge&m){
/*$TET$merger$e*/
		out.i = _in->i;
		_in->send(); out.send();
/*$TET$*/
	}

/*$TET$merger$$code&data*/
	void merge() {
		if (!(access(_in) && access(out)))return;

		if (is_first) {
			is_first = false;	j = _in->i;
			_in->send();
		}
		else {
			e.i = j; // request to the service for merging blocks e.i and e.j
			e.j = _in->i;
			e.send();
		}
	}

	int  j;
	bool is_first;
	mes* _in;
/*$TET$*/
};

#pragma templet *stopper(in?mes)

struct stopper : actor{
	enum tag{START,TAG_in};

	stopper(my_engine&e){
		TEMPLET::init(this, &e, stopper_recv_adapter);
/*$TET$stopper$stopper*/
/*$TET$*/
	}

	bool access(message*m){ return TEMPLET::access(m, this); }
	bool access(message&m){ return TEMPLET::access(&m, this); }

	void at(int _at){ TEMPLET::at(this, _at); }
	void delay(double t){ TEMPLET::delay(this, t); }
	double time(){ return TEMPLET::time(this); }
	void stop(){ TEMPLET::stop(this); }

	void in(mes&m){m._server_id=TAG_in; m._srv=this;}

	static void stopper_recv_adapter (actor*a, message*m, int tag){
		switch(tag){
			case TAG_in: ((stopper*)a)->in_handler(*((mes*)m)); break;
		}
	}

	void in_handler(mes&m){
/*$TET$stopper$in*/
		stop();
/*$TET$*/
	}

/*$TET$stopper$$code&data*/
/*$TET$*/
};

int main(int argc, char *argv[])
{
	my_engine e(argc, argv);
/*$TET$footer*/

	system("uname -a");

	cout << endl << "NUM_BLOCKS = " << NUM_BLOCKS << endl
		<< "BLOCK_SIZE = " << BLOCK_SIZE << endl
		<< "OMP_NUM_PROCS = " << omp_get_num_procs() << endl;

	omp_set_num_threads(1);
	everest an_everest(e);
	timer a_timer(e);

	an_everest.timer(a_timer.p);

	producer a_producer(e);
	stopper a_stoper(e);

	sorter** a_sorter = new sorter*[NUM_BLOCKS];
	for (int i = 0; i < NUM_BLOCKS; i++) {
		a_sorter[i] = new sorter(e);
		a_sorter[i]->i = i;
		a_producer.in(a_sorter[i]->out);
		an_everest.s(a_sorter[i]->e);
	}

	merger** a_merger = new merger*[NUM_BLOCKS - 1];
	for (int i = 0; i<NUM_BLOCKS - 1; i++) {
		a_merger[i] = new merger(e);
		an_everest.m(a_merger[i]->e);
	}

	mes* prev = &a_producer.out;
	for (int i = 0; i<NUM_BLOCKS - 1; i++) {
		a_merger[i]->in(*prev);
		prev = &(a_merger[i]->out);
	}
	a_stoper.in(*prev);

	double time = omp_get_wtime();
	e.run();
	time = omp_get_wtime() - time;

	cout << endl << "Block-sort time is " << time << " sec" << endl;

	return 0;

/*$TET$*/
}