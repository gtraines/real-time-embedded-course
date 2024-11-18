typedef struct {
	pthread_mutex_t		guard_mutex;
	pthread_cond_t		read_condvar;
	pthread_cond_t		write_condvar;
	int					rw_count; //-1 indicates a writer is active
	int					waiting_reader_count;
} rwlock_t;

acquire_write_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	while (rwlock->rw_count != 0)
		pthread_cond_wait (&rwlock->write_condvar, &rwlock->guard_mutex);
	rwlock->rw_count = -1;
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

release_write_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	rwlock->rw_count = 0;
	if (rwlock->waiting_reader_count) {
		for (int i = rwlock->waiting_reader_count; i>0; i--)
			pthread_cond_signal (&rwlock->read_condvar);
	} else
		//writers has lower priority than readers
		pthread_cond_signal (&rwlock->write_condvar);
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

acquire_read_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	rwlock->waiting_reader_count ++;
	while (rwlock->rw_count < 0)
		pthread_cond_wait (&rwlock->read_condvar, &rwlock->guard_mutex);
	rwlock->waiting_reader_count --;
	rwlock->rw_count ++;
	pthread_mutex_unlock (&rwlock->guard_mutex);
}

release_read_privilege(rwlock_t *rwlock) {
	pthread_mutex_lock(&rwlock->guard_mutex);
	rwlock->rw_count --;
	if (rwlock->rw_count == 0)
		//the last reader sends signal to a waiting writer
		pthread_cond_signal (&rwlock->write_condvar);
	pthread_mutex_unlock (&rwlock->guard_mutex);
}







