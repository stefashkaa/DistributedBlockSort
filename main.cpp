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

// 1
const int NUM_BLOCKS = 2;
const int BLOCK_SIZE = 64000000;
// 2
//const int NUM_BLOCKS = 4;
//const int BLOCK_SIZE = 32000000;
// 3
//const int NUM_BLOCKS = 8;
//const int BLOCK_SIZE = 16000000;
// 4
//const int NUM_BLOCKS = 16;
//const int BLOCK_SIZE = 8000000;
// 5
//const int NUM_BLOCKS = 32;
//const int BLOCK_SIZE = 4000000;
// 6
//const int NUM_BLOCKS = 64;
//const int BLOCK_SIZE = 2000000;
// 7
//const int NUM_BLOCKS = 128;
//const int BLOCK_SIZE = 1000000;

Everest *everestAPI;

int block_array[NUM_BLOCKS*BLOCK_SIZE];

void block_sort(int block_num)
{
	std::sort(&block_array[block_num*BLOCK_SIZE], &block_array[(block_num + 1)*BLOCK_SIZE]);
}

void block_merge(int less_block_num, int more_block_num)
{
	int* tmp_array = (int*)malloc(sizeof(int)*(2 * BLOCK_SIZE));

	if (!tmp_array) {
		std::cout << "Memory allocation failed!!!\n";
		exit(1);
	}

	std::merge(&block_array[less_block_num*BLOCK_SIZE], &block_array[(less_block_num + 1)*BLOCK_SIZE],
		&block_array[more_block_num*BLOCK_SIZE], &block_array[(more_block_num + 1)*BLOCK_SIZE],
		&tmp_array[0]);
	std::copy(&tmp_array[0], &tmp_array[BLOCK_SIZE], &block_array[less_block_num*BLOCK_SIZE]);
	std::copy(&tmp_array[BLOCK_SIZE], &tmp_array[2 * BLOCK_SIZE], &block_array[more_block_num*BLOCK_SIZE]);

	free(tmp_array);
}

bool is_sorted()
{
	int prev = block_array[0];
	for (int i = 1; i < NUM_BLOCKS*BLOCK_SIZE; i++) {
		if (prev > block_array[i])return false;
		prev = block_array[i];
	}
	return true;
}

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
		::init(this, argc, argv);
	}
	void run(){ TEMPLET::run(this); }
	void map(){ TEMPLET::map(this); }
};

#pragma templet ~mes=

struct mes : message{
	mes(actor*a, engine*e, int t) : _where(CLI), _cli(a), _client_id(t){
		::init(this, a, e);
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
		::init(this, a, e);
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
		::init(this, a, e);
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
		::init(this, &e, everest_recv_adapter);
/*$TET$everest$everest*/

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
				default: std::cout << "unknown task type\n";
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
				block_sort(eq->sort->i);
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
				block_merge(eq->merge->i, eq->merge->j);
				eq->done = true;
			}
		}
		m.send();
/*$TET$*/
	}

/*$TET$everest$$code&data*/
	~everest() {
		everestAPI->removeAccessToken();
		std::cout << "\n everest clean-up \n";
	}

	std::list<task_sort*>  tsort;
	std::list<task_merge*> tmerge;
	std::list<everest_queue*> queue;

/*$TET$*/
};

#pragma templet *timer(p!mes)+

struct timer : actor{
	enum tag{START,TAG_p};

	timer(my_engine&e):p(this, &e, TAG_p){
		::init(this, &e, timer_recv_adapter);
		::init(&_start, this, &e);
		::send(&_start, this, START);
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
		::init(this, &e, sorter_recv_adapter);
		::init(&_start, this, &e);
		::send(&_start, this, START);
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
		::init(this, &e, producer_recv_adapter);
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
		if (!bc) out_handler(m);
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
		::init(this, &e, merger_recv_adapter);
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
			//block_merge(j,_in->i);
			//out.i = _in->i;
			//_in->send();out.send();
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
		::init(this, &e, stopper_recv_adapter);
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

	everestAPI = new Everest();
	cout << "Access token: " << everestAPI->getAccessToken("stefanpopov", "qwaszx1", "blocksort") << endl;

	std::cout << "\nNUM_BLOCKS = " << NUM_BLOCKS << endl
		<< "BLOCK_SIZE = " << BLOCK_SIZE << endl
		<< "OMP_NUM_PROCS = " << omp_get_num_procs() << endl;

	srand(1); for (int i = 0; i < NUM_BLOCKS*BLOCK_SIZE; i++)	block_array[i] = rand();

	double time = omp_get_wtime();
	std::sort(&block_array[0], &block_array[NUM_BLOCKS*BLOCK_SIZE]);
	time = omp_get_wtime() - time;

	std::cout << "\nSequential sort time is " << time << " sec\n";

	//////////////////// sequential blocksort /////////////////////
	srand(1); for (int i = 0; i < NUM_BLOCKS*BLOCK_SIZE; i++)	block_array[i] = rand();

	time = omp_get_wtime();

	for (int i = 0; i<NUM_BLOCKS; i++) block_sort(i);
	for (int i = 1; i<NUM_BLOCKS; i++) for (int j = 0; j<i; j++) block_merge(j, i);

	time = omp_get_wtime() - time;

	if (!is_sorted())std::cout << "\nSomething went wrong in the sequential block-sort!!!\n";
	else std::cout << "Sequential block-sort time is " << time << " sec\n";
	///////////////////////////////////////////////////////////////

	/////////////////// parallel actor blocksort //////////////////
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

	srand(1); for (int i = 0; i < NUM_BLOCKS*BLOCK_SIZE; i++)	block_array[i] = rand();

	time = omp_get_wtime();
	e.run();
	time = omp_get_wtime() - time;

	if (!is_sorted())std::cout << "\nSomething went wrong in the parallel actor block-sort!!!\n";
	else std::cout << "\nParallel block-sort time is " << time << " sec\n";
	system("pause");
	return 0;

/*$TET$*/
}