#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N_ACCOUNTS 10
#define N_THREADS  20
#define N_ROUNDS   10000

/* 10 accounts with $100 apiece means there's $1,000
   in the system. Let's hope it stays that way...  */
#define INIT_BALANCE 100

/* making a struct here for the benefit of future
   versions of this program */
struct account
{
  long balance;
  pthread_mutex_t lock; /* mutex to protect the account */
} accts[N_ACCOUNTS];

/* Helper for bankers to choose an account and amount at
   random. It came from Steve Summit's excellent C FAQ
   http://c-faq.com/lib/randrange.html */
int rand_range(int N)
{
  return (int)((double)rand() / ((double)RAND_MAX + 1) * N);
}

/* each banker will run this function concurrently. The
   weird signature is required for a thread function */
void *disburse(void *arg)
{
  size_t i, from, to;
  long payment;

  for (i = 0; i < N_ROUNDS; i++)
    {
      /* pick distinct 'from' and 'to' accounts */
      from = rand_range(N_ACCOUNTS);
      do {
	      to = rand_range(N_ACCOUNTS);
      } while (to == from);

      /* acquire locks for both accounts before updating them */
      pthread_mutex_lock(&accts[from].lock);
      pthread_mutex_lock(&accts[to].lock);

      /* go nuts sending money, try not to overdraft */
      if (accts[from].balance > 0)
	{
	  payment = 1 + rand_range(accts[from].balance);
	  accts[from].balance -= payment;
	  accts[to].balance   += payment;
	}

      /* release locks for both accounts after updating them */
      pthread_mutex_unlock(&accts[from].lock);
      pthread_mutex_unlock(&accts[to].lock);
    }
  return NULL;
}

int main(void)
{
  size_t i;
  long total;
  pthread_t bankers[N_THREADS];

  srand(time(NULL));

  for (i = 0; i < N_ACCOUNTS; i++) {
    accts[i].balance = INIT_BALANCE;
    pthread_mutex_init(&accts[i].lock, NULL); /* initialize the mutex for the account */
    printf("Account %ld balance = $%ld\n", i, accts[i].balance);
  }
	
  printf("Initial money in system: %d\n\n",
	 N_ACCOUNTS * INIT_BALANCE);

  /* start the threads, using whatever parallelism the
     system happens to offer. Note that pthread_create
     is the *only* function that creates concurrency */
  for (i = 0; i < N_THREADS; i++) {
    pthread_create(&bankers[i], NULL, disburse, NULL);
  }

  /* wait for the threads to all finish, using the
     pthread_t handles pthread_create gave us */
  for (i = 0; i < N_THREADS; i++) {
    pthread_join(bankers[i], NULL);
  }

  for (total = 0, i = 0; i < N_ACCOUNTS; i++) {
    total += accts[i].balance;
    printf("Account %ld balance = $%ld\n", i, accts[i].balance); 
    pthread_mutex_destroy(&accts[i].lock); /* destroy the mutex for the account */
  }

  printf("Final money in system: %ld\n", total);
}