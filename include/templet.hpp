/*--------------------------------------------------------------------------*/
/*  Copyright 2010-2016 Sergei Vostokin                                     */
/*                                                                          */
/*  Licensed under the Apache License, Version 2.0 (the "License");         */
/*  you may not use this file except in compliance with the License.        */
/*  You may obtain a copy of the License at                                 */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*/
/*  See the License for the specific language governing permissions and     */
/*  limitations under the License.                                          */
/*--------------------------------------------------------------------------*/

#pragma once

namespace TEMPLET{

	struct actor;
	struct message;
	struct engine;

	struct saver;
	struct restorer;

	inline void init(actor*, engine*, void(*recv)(actor*,message*,int tag), void(*save)(actor*,saver*)=0, void(*restore)(actor*,restorer*)=0);
	inline bool at(actor*, int node);
	inline void stop(actor*);
	inline void delay(actor*, double);
	inline double time(actor*);


	inline void init(message*, actor*, engine*, void(*save)(message*,saver*)=0, void(*restore)(message*, restorer*)=0);
	inline void send(message*, actor*,int tag);
	inline bool access(message*, actor*);

	inline void init(engine*, int argc = 0, char *argv[] = 0);
	inline int  nodes(engine*);
	inline void map(engine*);
	inline void run(engine*);
	inline bool stat(engine*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp);

	inline void save(saver*, void*, size_t);
	inline void restore(restorer*, void*, size_t);
}

namespace TEMPLET {
	struct engine_interface {
		engine_interface(int argc, char *argv[]) {}
		void run() {}
		void map() {}
	};

	struct message_interface { void send() {} };

	struct actor_interface {
		bool access(message_interface*) { return false; }
		bool access(message_interface&) { return false; }
		void delay(double) {}
		double time() { return 0; }
		void at(int) {} 
		void stop() {} 
	};
}

#if defined(DEBUG_EXECUTION) || (!defined(SERIAL_EXECUTION) && !defined(PARALLEL_EXECUTION) && !defined(SIMULATED_EXECUTION) && !defined(DISTRIBUTED_EXECUTION))

#ifndef DEBUG_EXECUTION
#define DEBUG_EXECUTION
#endif

#include <vector>
#include <assert.h>

#ifdef DEBUG_SERIALIZATION

#include <memory.h>
#define ALLOC_SIZE		4096

#endif	

namespace TEMPLET{

	struct actor{
		void(*_recv)(actor*,message*,int tag);
		engine* _engine;
#ifdef DEBUG_SERIALIZATION
		void(*_save)(actor*,saver*);
		void(*_restore)(actor*,restorer*);
#endif
	};

	struct message{
		actor* _actor;
		bool _sending;
#ifdef DEBUG_SERIALIZATION
		void(*_save)(message*,saver*);
		void(*_restore)(message*,restorer*);
#endif
		int _tag;
	};

#ifdef DEBUG_SERIALIZATION
	struct saver{ engine*_engine; };
	struct restorer{ engine*_engine; };
#endif

	struct engine{
		std::vector<message*> _ready;
		bool _stop;
#ifdef DEBUG_SERIALIZATION
		void* _buffer;
		size_t _buffer_size;
		size_t _buffer_cursor;
		saver _saver;
		restorer _restorer;
#endif		
	};

	inline void init(actor*a, engine*e, void(*recv)(actor*, message*, int tag), void(*save)(actor*, saver*), void(*restore)(actor*, restorer*))
	{
		a->_recv = recv; a->_engine = e;
#ifdef DEBUG_SERIALIZATION
		a->_save = save; 
		a->_restore = restore;
#endif
	}

	inline bool at(actor*, int node){ return false; }
	inline void stop(actor*a){	a->_engine->_stop = true; }
	inline void delay(actor*, double){}
	inline double time(actor*) { return 0.0; }

	inline void init(message*m, actor*a, engine*e, void(*save)(message*,saver*), void(*restore)(message*, restorer*))
	{
		m->_sending = false; m->_actor = a; m->_tag = 0;
#ifdef DEBUG_SERIALIZATION
		m->_save = save;
		m->_restore = restore;
#endif
	}

	inline void send(message*m, actor*a,int tag)
	{
		assert(!m->_sending);

		engine* e = a->_engine;
		m->_sending = true;
		m->_actor = a;
		m->_tag = tag;
		e->_ready.push_back(m);
	}

	inline bool access(message*m, actor*a)
	{
		return m->_actor == a && !m->_sending;
	}

	inline void init_buffer(engine*e);

	inline void init(engine*e, int argc, char *argv[])
	{
		e->_ready.clear();
		e->_stop = false;
		init_buffer(e);
	}
	
	inline int  nodes(engine*){ return 1; }
	inline void map(engine*){}

	inline void run(engine*e)
	{
		size_t rsize;
		while ((rsize = e->_ready.size())){
			int n = rand() % rsize;	std::vector<message*>::iterator it = e->_ready.begin() + n;
			message* m = *it; e->_ready.erase(it); m->_sending = false;
			actor* a = m->_actor;

#ifdef DEBUG_SERIALIZATION
			if (m->_save){
				e->_buffer_cursor = 0;
				m->_save(m,&e->_saver);
			}
			if (m->_restore){
				e->_buffer_cursor = 0;
				m->_restore(m,&e->_restorer);
			}
			if (m->_actor->_save){
				e->_buffer_cursor = 0;
				m->_actor->_save(m->_actor,&e->_saver);
			}
			if (m->_actor->_restore){
				e->_buffer_cursor = 0;
				m->_actor->_restore(m->_actor,&e->_restorer);
			}
#endif
			a->_recv(a,m,m->_tag);
			if (e->_stop) break;
		}
		assert(e->_stop);
	}

	inline bool stat(void*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
	inline bool stat(engine*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
}
#elif defined(SERIAL_EXECUTION)

#include <queue>

namespace TEMPLET{

	struct actor{
		void(*_recv)(actor*, message*, int tag);
		engine* _engine;
	};

	struct message{
		actor* _actor;
		bool _sending;
		int _tag;
	};

	struct engine{
		std::queue<message*> _ready;
		bool _stop;
	};

	inline void init(actor*a, engine*e, void(*recv)(actor*, message*, int tag), void(*save)(actor*, saver*), void(*restore)(actor*, restorer*))
	{
		a->_recv = recv; a->_engine = e;
	}

	inline bool at(actor*, int node){ return false; }
	inline void stop(actor*a){ a->_engine->_stop = true; }
	inline void delay(actor*, double){}
	inline double time(actor*) { return 0.0; }

	inline void init(message*m, actor*a, engine*e, void(*save)(message*, saver*), void(*restore)(message*, restorer*))
	{
		m->_sending = false; m->_actor = a; m->_tag = 0;
	}

	inline void send(message*m, actor*a, int tag)
	{
		if (m->_sending) return;
		engine* e = a->_engine;
		
		m->_sending = true;
		m->_actor = a;
		m->_tag = tag;
		e->_ready.push(m);
	}

	inline bool access(message*m, actor*a)
	{
		return m->_actor == a && !m->_sending;
	}

	inline void init(engine*e, int argc, char *argv[])
	{
		while (!e->_ready.empty())e->_ready.pop();
		e->_stop = false;
	}

	inline int  nodes(engine*){ return 1; }
	inline void map(engine*){}

	inline void run(engine*e)
	{
		while (!e->_ready.empty()){
			message*m = e->_ready.front(); e->_ready.pop();
			actor*a = m->_actor;
			m->_sending = false;
			a->_recv(a,m,m->_tag);
			if (e->_stop) break;
		}
	}

	inline bool stat(void*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
	inline bool stat(engine*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
}
#elif defined(PARALLEL_EXECUTION)

#if !defined(USE_OPENMP)

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

#else

#include <omp.h>
#include <queue>

namespace std {
	struct mutex {
		mutex() { omp_init_lock(&_lock); }
		~mutex() { omp_destroy_lock(&_lock); }
		omp_lock_t _lock;
	};

	template <class T>
	struct unique_lock {
		unique_lock(T&m) :_mutex(m) { 
		    //while (!omp_test_lock(&(_mutex._lock))); 
		    omp_set_lock(&(_mutex._lock));
		}
		~unique_lock() { omp_unset_lock(&(_mutex._lock)); }
		T& _mutex;
	};

	struct condition_variable {
		condition_variable() :_signal(0) { }
		~condition_variable() { }
		void notify_one() {_signal = 1;}
		void wait(unique_lock<mutex>&m) {
			_signal = 0;
			omp_unset_lock(&m._mutex._lock);
			while (!_signal);
			while (!omp_test_lock(&m._mutex._lock));
		}
		char _reserved1[64];
		volatile int _signal;
		char _reserved2[64];
	};
}

#endif

namespace TEMPLET{

	struct actor{
		void(*_recv)(actor*, message*, int tag);
		std::mutex _mtx;
		engine* _engine;
	};

	struct message{
		actor* _actor;
		bool _sending;
		int _tag;
	};

	struct engine{
		volatile int _active;
		std::mutex _mtx;
		std::condition_variable _cv;
		std::queue<message*> _ready;
		volatile bool _stop;
	};

	inline void init(actor*a, engine*e, void(*recv)(actor*, message*, int tag), void(*save)(actor*, saver*), void(*restore)(actor*, restorer*))
	{
		a->_recv = recv; a->_engine = e;
	}

	inline bool at(actor*, int node){ return false; }
	inline void delay(actor*, double){}
	inline double time(actor*) { return 0.0; }
	
	inline void stop(actor*a)
	{ 
		engine*e = a->_engine;
		std::unique_lock<std::mutex> lck(e->_mtx);
		e->_stop = true; 
	}

	inline void init(message*m, actor*a, engine*e, void(*save)(message*, saver*), void(*restore)(message*, restorer*))
	{
		m->_sending = false; m->_actor = a; m->_tag = 0;
	}

	inline void send(message*m, actor*a, int tag)
	{
		if (m->_sending) return;
		engine* e = a->_engine;

		m->_sending = true;
		m->_actor = a;
		m->_tag = tag;

		std::unique_lock<std::mutex> lck(e->_mtx);

		if (e->_ready.empty()) {e->_ready.push(m); e->_cv.notify_one();	}
		else e->_ready.push(m);
	}

	inline bool access(message*m, actor*a)
	{
		return m->_actor == a && !m->_sending;
	}

	inline void init(engine*e, int argc, char *argv[])
	{
		while (!e->_ready.empty())e->_ready.pop();
		e->_stop = false;
	}

	inline int  nodes(engine*){ return 1; }
	inline void map(engine*){}

	inline void tfunc(engine*e)
	{
		message*m; 
		actor*a;

		for (;;){
			{
				std::unique_lock<std::mutex> lck(e->_mtx);
				
				if (e->_stop) { e->_cv.notify_one(); return; }
				
				while (e->_ready.empty()){
					e->_active--;
					if (!e->_active){ e->_cv.notify_one(); return; }
					e->_cv.wait(lck); 
					if (e->_stop) { e->_cv.notify_one(); return; }
					e->_active++;
				}
				m = e->_ready.front();
				e->_ready.pop();
			}
			a = m->_actor;
			{
				std::unique_lock<std::mutex> lck(a->_mtx);
				m->_sending = false;
				a->_recv(a, m, m->_tag);
			}
		}
	}

	inline void run(engine*e)
	{
#if defined(USE_OPENMP)
int n = omp_get_max_threads();
omp_set_num_threads(n+1);
#pragma omp parallel 
		{
#pragma omp single
			{
				e->_active = omp_get_num_threads();
				tfunc(e);
			}
		}
#else
		unsigned n = std::thread::hardware_concurrency();
		std::vector<std::thread> threads(n);
		e->_active = n;
		for (unsigned i = 0; i<n; i++) threads[i] = std::thread(tfunc, e);
		for (auto& th : threads) th.join();
#endif
	}

	inline bool stat(void*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
	inline bool stat(engine*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
}
#elif defined(SIMULATED_EXECUTION)

#include <vector>
#include <queue>

namespace TEMPLET{
	
	struct event{
		double _time;
		enum{ MESSAGE, ACTOR } _type;
		message*_message;
		actor*_actor;
	};

	class cmp{ public: bool operator()(const event&t1, const event&t2){ return t1._time > t2._time; } };

	struct actor{
		void(*_recv)(actor*, message*, int tag);
		engine* _engine;
		bool _lck;
		std::queue<message*> _ready;
	};

	struct message{
		actor* _actor;
		bool _sending;
		int _tag;
	};

	struct engine{
		std::priority_queue<event, std::vector<event>, cmp> _calendar;
		double _Tp;
		double _T1;
		int _Pmax;
		bool _stop;
	};

	inline void init(actor*a, engine*e, void(*recv)(actor*, message*, int tag), void(*save)(actor*, saver*), void(*restore)(actor*, restorer*))
	{
		a->_recv = recv; a->_engine = e; a->_lck = false;
		while (!a->_ready.empty())a->_ready.pop();
	}

	inline bool at(actor*, int node){ return false; }
	inline void stop(actor*a){ a->_engine->_stop = true; }

	inline void delay(actor*a, double t)
	{
		engine*e = a->_engine;
		e->_T1 += t;
		e->_Tp += t;
	}

	inline double time(actor*a)
	{ 
		engine*e = a->_engine; 
		return e->_Tp; 
	}

	inline void init(message*m, actor*a, engine*e, void(*save)(message*, saver*), void(*restore)(message*, restorer*))
	{
		m->_sending = false; m->_actor = a; m->_tag = 0;
	}

	inline void send(message*m, actor*a, int tag)
	{
		if (m->_sending) return;
		engine* e = a->_engine;

		m->_sending = true;	m->_actor = a; m->_tag = tag;

		event ev;
		ev._time = e->_Tp; ev._type = event::MESSAGE; ev._message = m;
		e->_calendar.push(ev);
	}

	inline bool access(message*m, actor*a)
	{
		return m->_actor == a && !m->_sending;
	}

	inline void init(engine*e, int argc, char *argv[])
	{
		while (!e->_calendar.empty())e->_calendar.pop();
		e->_stop = false;
		e->_Tp = e->_T1 = 0.0;
		e->_Pmax = 0;
		e->_stop = false;
	}

	inline int  nodes(engine*){ return 1; }
	inline void map(engine*){}

	inline void run(engine*e)
	{
		actor*a = 0; message*m = 0;
		double Tcur = 0.0, Tprev = 0.0;
		int Pcur = 0, Pmax = 0;

		while (!e->_calendar.empty()){
			event ev;
			ev = e->_calendar.top();	e->_calendar.pop();

			Tcur = ev._time;
			if (Tcur - Tprev > 0 && Pcur > Pmax) Pmax = Pcur;
			Tprev = Tcur;

			switch (ev._type){

			case event::MESSAGE:
			{
				m = ev._message;
				a = ev._message->_actor;

				if (a->_lck)
					a->_ready.push(m);
				else{
					m->_sending = false;

					a->_lck = true; Pcur++;

					e->_Tp = Tcur; a->_recv(a,m,m->_tag); Tcur = e->_Tp;

					if (e->_stop){ e->_Tp = Tcur; e->_Pmax = Pmax; return; }

					ev._time = Tcur; ev._type = event::ACTOR; ev._actor = a;
					e->_calendar.push(ev);
				}
			}
			break;
			case event::ACTOR:
			{
				a = ev._actor;
				if (a->_ready.empty()){
					a->_lck = false; Pcur--;
				}
				else{
					m = a->_ready.front(); a->_ready.pop();
					m->_sending = false;

					e->_Tp = Tcur; a->_recv(a,m,m->_tag); Tcur = e->_Tp;

					if (e->_stop){ e->_Tp = Tcur; e->_Pmax = Pmax; return; }

					ev._time = Tcur; ev._type = event::ACTOR; ev._actor = a;
					e->_calendar.push(ev);
				}
			}
			}
		}
		e->_Tp = Tcur; e->_Pmax = Pmax;
	}

	inline bool stat(engine*e, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp)
	{
		*T1 = e->_T1; *Tp = e->_Tp; *Pmax = e->_Pmax;
		*Smax = *T1 / *Tp;
		double alfa = (1 - *Smax / *Pmax) / (*Smax - *Smax / *Pmax);
		*Sp = (P > *Pmax) ? *Smax : 1 / (alfa + (1 - alfa) / P);
		return true;
	}
}
#elif defined(DISTRIBUTED_EXECUTION)

#include <mpi.h>
#include <assert.h>
#include <queue>
#include <memory.h>

#define ALLOC_SIZE				4096

#define MASTER					0

#define TAG_ACTOR				0
#define TAG_MESSAGE				1
#define TAG_STOP_EQUEST			2
#define TAG_STOP_ACKNOWLEDGED	3

namespace TEMPLET{
	
	struct actor{
		void(*_recv)(actor*, message*, int tag);
		engine* _engine;
		int _mpi_rank;
		int _id;
		void(*_save)(actor*, saver*);
		void(*_restore)(actor*, restorer*);
	};

	struct message{
		actor* _actor;
		bool _sending;
		int _id;
		void(*_save)(message*, saver*);
		void(*_restore)(message*, restorer*);
		int _tag;
	};

	struct saver{ engine*_engine; };
	struct restorer{ engine*_engine; };

	struct engine{
		bool _stop;
		bool _running;
		int _mpi_size;
		int _mpi_rank;
		std::vector<actor*> _actors;
		std::vector<message*> _messages;
		std::queue<message*> _ready;
		void* _buffer;
		size_t _buffer_size;
		size_t _buffer_cursor;
		saver _saver;
		restorer _restorer;
	};

	inline void init(actor*a, engine*e, void(*recv)(actor*, message*, int tag), void(*save)(actor*, saver*), void(*restore)(actor*, restorer*))
	{
		a->_recv = recv; a->_engine = e;
		a->_save = save;
		a->_restore = restore;
		a->_mpi_rank = 0;
		a->_id = (int)e->_actors.size();
		e->_actors.push_back(a);
	}

	inline bool at(actor*a, int node)
	{
		engine*e = a->_engine;
		if (e->_running || node > e->_mpi_size - 1) return false;
		else { a->_mpi_rank = node; return true; }
	}
	
	inline void stop(actor*a)
	{
		int stub;
		MPI_Send(&stub, sizeof(stub), MPI_INT, MASTER, TAG_STOP_EQUEST, MPI_COMM_WORLD);
	}
	
	inline void delay(actor*, double){}
	inline double time(actor*) { return 0.0; }

	inline void init(message*m, actor*a, engine*e, void(*save)(message*, saver*), void(*restore)(message*, restorer*))
	{
		m->_sending = false; m->_actor = a; m->_tag = 0;
		m->_save = save;
		m->_restore = restore;
		m->_id = (int)e->_messages.size();
		e->_messages.push_back(m);
	}

	inline void send(message*m, actor*a, int tag)
	{
		engine* e = a->_engine;
		m->_sending = true;
		m->_actor = a;
		m->_tag = tag;
		e->_ready.push(m);
	}

	inline bool access(message*m, actor*a)
	{
		return m->_actor == a && !m->_sending;
	}

	inline void init_buffer(engine*e);

	inline void init(engine*e, int argc, char *argv[])
	{
		while(!e->_ready.empty())e->_ready.pop();
		e->_actors.clear();
		e->_messages.clear();
		e->_stop = false;
		init_buffer(e);

		MPI_Init(&argc, &argv);
		MPI_Comm_size(MPI_COMM_WORLD, &e->_mpi_size);
		MPI_Comm_rank(MPI_COMM_WORLD, &e->_mpi_rank);

		e->_running = false;
	}

	inline int  nodes(engine*e)	{	return e->_mpi_size;	}

	inline void send_actor(actor*a, int rank)
	{
		engine*e = a->_engine;
		e->_buffer_cursor = 0;
		save(&e->_saver, &a->_id, sizeof(a->_id));
		a->_save(a, &e->_saver);
		MPI_Send(e->_buffer, (int)e->_buffer_cursor, MPI_BYTE, rank, TAG_ACTOR, MPI_COMM_WORLD);
	}

	inline void recv_actor(engine*e, int rank)
	{	
		int buffer_size;
		MPI_Status status;
		
		int actor_id;
		actor*a;
	
		MPI_Probe(rank, TAG_ACTOR, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_BYTE, &buffer_size);

		if (size_t(buffer_size) > e->_buffer_size)
			e->_buffer = realloc(e->_buffer, ALLOC_SIZE*(buffer_size / ALLOC_SIZE + 1));

		MPI_Recv(e->_buffer, buffer_size, MPI_BYTE, rank, TAG_ACTOR, MPI_COMM_WORLD, &status);

		e->_buffer_cursor = 0;
		restore(&e->_restorer, &actor_id, sizeof(actor_id));
		a = e->_actors[actor_id];
		a->_restore(a,&e->_restorer);
	}

	inline void send_message(message*m,engine*e,int rank)
	{
		e->_buffer_cursor = 0;
	
		save(&e->_saver, &m->_id, sizeof(m->_id));
		save(&e->_saver, &m->_actor->_id, sizeof(m->_actor->_id));
		save(&e->_saver, &m->_tag, sizeof(m->_tag));

		m->_save(m,&e->_saver);

		MPI_Send(e->_buffer, (int)e->_buffer_cursor, MPI_BYTE, rank, TAG_MESSAGE, MPI_COMM_WORLD);
	}

	inline void recv_message(engine*e)
	{	
		int buffer_size;
		MPI_Status status;
		
		int message_id;
		int actor_id;

		message*m;
		actor*a;
		int tag;

		MPI_Probe(MPI_ANY_SOURCE, TAG_MESSAGE, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_BYTE, &buffer_size);

		if (size_t(buffer_size) > e->_buffer_size)
			e->_buffer = realloc(e->_buffer, ALLOC_SIZE*(buffer_size / ALLOC_SIZE + 1));

		MPI_Recv(e->_buffer, buffer_size, MPI_BYTE, MPI_ANY_SOURCE, TAG_ACTOR, MPI_COMM_WORLD, &status);

		e->_buffer_cursor = 0;
		
		restore(&e->_restorer, &message_id, sizeof(message_id));
		m = e->_messages[message_id];

		restore(&e->_restorer, &actor_id, sizeof(actor_id));
		a = e->_actors[actor_id];

		restore(&e->_restorer, &tag, sizeof(tag));

		m->_restore(m, &e->_restorer);

		send(m, a, tag);
	}

	inline int num_of_actors_with_rank(engine*e,int rank)
	{
		int num = 0;
		for (actor*a : e->_actors)	if (a->_mpi_rank == rank) num++;
		return num;
	}

	inline void map(engine*e)
	{
		int stub;
		MPI_Status status;

		if (e->_mpi_rank == MASTER) return;
		e->_running = true;
		
		int my_rank = e->_mpi_rank;
		int num_local = num_of_actors_with_rank(e, my_rank);

		for (int i = 0; i < num_local; i++){
			recv_actor(e, MASTER);
		}

		for (;;){
			if (!e->_ready.empty()){
				message* m = e->_ready.front();
				e->_ready.pop();

				int dest_rank = m->_actor->_mpi_rank;

				if (dest_rank == e->_mpi_rank){
					actor*a = m->_actor;
					m->_sending = false;
					a->_recv(a, m, m->_tag);
				}
				else
					send_message(m, e, dest_rank);
			}
			else{
				MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

				if (status.MPI_TAG == TAG_MESSAGE)	recv_message(e);
				else if (status.MPI_TAG == TAG_STOP_ACKNOWLEDGED){
					e->_running = false;
					MPI_Recv(&stub, sizeof(stub), MPI_INT, MASTER, TAG_STOP_ACKNOWLEDGED, MPI_COMM_WORLD, &status);
					break;
				}
			}
		}

		for (actor*a : e->_actors)
			if (a->_mpi_rank == e->_mpi_rank)
				send_actor(a, MASTER);

		MPI_Finalize();
		exit(0);
	}

	inline void run(engine*e)
	{
		int stub;
		MPI_Status status;
		e->_running = true;
		
		for (actor*a : e->_actors)
			if (a->_mpi_rank != MASTER)
				send_actor(a, a->_mpi_rank);
			
		for (;;){
			if (!e->_ready.empty()){
				message* m = e->_ready.front();
				e->_ready.pop();

				int dest_rank = m->_actor->_mpi_rank;

				if (dest_rank == MASTER){
					actor*a = m->_actor;
					m->_sending = false;
					a->_recv(a, m, m->_tag);
				}
				else
					send_message(m, e, dest_rank);
			}
			else{
				MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

				if (status.MPI_TAG == TAG_MESSAGE)	recv_message(e);
				else if (status.MPI_TAG == TAG_STOP_EQUEST){
					e->_running = false;
					MPI_Recv(&stub, sizeof(stub), MPI_INT, MPI_ANY_SOURCE, TAG_STOP_EQUEST, MPI_COMM_WORLD, &status);
					break;
				}
			}
		}

		int not_local_actors_num = (int)e->_actors.size() - num_of_actors_with_rank(e, MASTER);

		for (int i = 0; i < not_local_actors_num; i++)
			recv_actor(e, MPI_ANY_SOURCE);
		
		MPI_Finalize();
	}

	inline bool stat(void*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
	inline bool stat(engine*, double*T1, double*Tp, int*Pmax, double*Smax, int P, double*Sp){ return false; }
}
#endif

#if (defined(DEBUG_SERIALIZATION) && defined(DEBUG_EXECUTION)) || defined(MPI_EXECUTION)

namespace TEMPLET{
	inline void init_buffer(engine*e)
	{
		e->_buffer_size = ALLOC_SIZE;
		e->_buffer = malloc(ALLOC_SIZE);
		assert(e->_buffer);
		e->_buffer_cursor = 0;
		e->_saver._engine = e;
		e->_restorer._engine = e;
	}

	inline void save(saver*s, void*source, size_t size)
	{
		engine* e = s->_engine;
		if (e->_buffer_size < size + e->_buffer_cursor){
			int blocks = (int)(size + e->_buffer_cursor) / ALLOC_SIZE + 1; 
			e->_buffer = realloc(e->_buffer, blocks*ALLOC_SIZE);
			assert(e->_buffer);
			e->_buffer_size = blocks * ALLOC_SIZE;
		}
		memmove((char*)(e->_buffer) + (e->_buffer_cursor), source, size);
		e->_buffer_cursor += size;
	}

	inline void restore(restorer*r, void*target, size_t size)
	{
		engine* e = r->_engine;
		memmove((char*)(target), (char*)(e->_buffer) + (e->_buffer_cursor), size);
		e->_buffer_cursor += size;
	}
}
#else
namespace TEMPLET{
	inline void init_buffer(engine*e){}
	inline void save(saver*, void*, size_t){}
	inline void restore(restorer*, void*, size_t){}
}
#endif