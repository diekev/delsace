#define _REENTRANT

#include <stdio.h>
#include <pthread.h>
#include <string.h> /* pour strerror */

/* Ce fichier montre la transformation du code suivant en coroutine sur un thread séparé :
 *
 * corout fn_coroutine() : z32
 * {
 *    pour i dans 0 ... 9 {
 *       retiens i
 *       printf("reprise coroutine...\n");
 *    }
 *
 *    printf("coroutine terminée\n");
 * }
 *
 * fonc appel_coroutine() : rien
 * {
 *    pour i dans fn_coroutine() {
 *       printf("i = %d\n", i);
 *    }
 * }
 *
 * La synchronisation entre les deux threads est inspirée de :
 * https://github.com/huanzai/pthread_coroutine/blob/master/coroutine.c
 */

/* L'état nous sert à communiquer, à passer les données, entre les deux threads */
typedef struct etat_coroutine {
   pthread_mutex_t mutex_boucle;
   pthread_cond_t cond_boucle;

   pthread_mutex_t mutex_coro;
   pthread_cond_t cond_coro;

   // données générales
   int termine;

   // paramètres de la coroutine

   // sorties de la coroutine
   int __ret0;
} etat_coroutine;

/* Code transformée de la coroutine. Ceci est passé à pthread_create.
 * data est l'état de la coroutine
 */
void *fn_coroutine(void *data)
{
   etat_coroutine *__etat = (etat_coroutine *)data;
   printf("lancement coroutine...\n");

   for (int i = 0; i < 10; ++i) {
      printf("suspension coroutine...\n");

      // retiens i
      pthread_mutex_lock(&__etat->mutex_coro);

      __etat->__ret0 = i;

      pthread_mutex_lock(&__etat->mutex_boucle);
      pthread_cond_signal(&__etat->cond_boucle);
      pthread_mutex_unlock(&__etat->mutex_boucle);

      pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);
      pthread_mutex_unlock(&__etat->mutex_coro);

      // continuation
      printf("reprise coroutine...\n");
   }

   __etat->termine = 1;
   pthread_mutex_lock(&__etat->mutex_boucle);
   pthread_cond_signal(&__etat->cond_boucle);
   pthread_mutex_unlock(&__etat->mutex_boucle);

   printf("coroutine terminée\n");

   return NULL;
}

/* Code transformée de la fonction appelant la coroutine.
 * On construit un état pour la coroutine, et on lance un thread pour calculer la coroutine.
 * Il faut synchronisé les deux threads, à savoir suspendre la coroutine quand elle atteint "retiens"
 * et la reprendre quand nous avons fini de lire les variables nous intéressants.
 * Également nous ne devons pas commencer à lire le résultat de la coroutine avant que celle-ci n'ait été déjà suspendue.
 */
void appel_coroutine()
{
   etat_coroutine etat = {
      .mutex_boucle = PTHREAD_MUTEX_INITIALIZER,
      .mutex_coro = PTHREAD_MUTEX_INITIALIZER,
      .cond_coro = PTHREAD_COND_INITIALIZER,
      .cond_boucle = PTHREAD_COND_INITIALIZER,
      .termine = 0
   };

   pthread_t fil_coro;
   int ret = pthread_create(&fil_coro, NULL, fn_coroutine, &etat);
   printf("fil créé\n");

   pthread_mutex_lock(&etat.mutex_boucle);
   pthread_cond_wait(&etat.cond_boucle, &etat.mutex_boucle);
   pthread_mutex_unlock(&etat.mutex_boucle);

   while (etat.termine == 0) {
      pthread_mutex_lock(&etat.mutex_boucle);

      int i = etat.__ret0;

      pthread_mutex_lock(&etat.mutex_coro);
      pthread_cond_signal(&etat.cond_coro);
      pthread_mutex_unlock(&etat.mutex_coro);

      printf("suspension boucle...\n");
      pthread_cond_wait(&etat.cond_boucle, &etat.mutex_boucle);

      pthread_mutex_unlock(&etat.mutex_boucle);

      // corps boucle
      printf("i = %d\n", i);
   }

   pthread_join(fil_coro, NULL);
   printf("fil joint\n");
}

int main (void)
{
   appel_coroutine();
   return 0;
}
