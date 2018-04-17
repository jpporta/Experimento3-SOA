/*******************************************************************************
*
* Este programa faz parte do curso sobre tempo real do Laboratorio Embry-Riddle
*
* Seguem os comentarios originais:
*
* Experiment #5: Semaphores
*
*    Programmer: Eric Sorton
*          Date: 3/17/97
*           For: MSE599, Special Topics Class
*
*       Purpose: The purpose of this program is to demonstrate how semaphores
*		 can be used to protect a critical region.  Its sole purpose
*		 is to print a character string (namely the alphabet) to the
*		 screen.  Any number of processes can be used to cooperatively
*		 (or non-cooperatively) print the string to the screen.  An
*		 index is stored in shared memory, this index is the index into
*		 the array that identifies which character within the string
*		 should be printed next.  Without semaphores, all the processes
*		 access this index simultaneously and conflicts occur.  With
*		 semahpores, the character string is displayed neatly to the
*		 screen.
*
*		 The optional semaphore protection can be compiled into them
*		 program using the MACRO definition of PROTECT.  To compile
*		 the semaphore protection into the program, uncomment the
*		 #define below.
*
*
*       Proposito: O proposito deste programa e o de demonstrar como semaforos
*		podem ser usados para proteger uma regiao critica. O programa exibe
*		um string de caracteres (na realidade um alfabeto). Um n�mero
*		qualquer de processos pode ser usado para exibir o string, seja
*		de maneira cooperativa ou nao cooperativa. Um indice e armazenado
*		em memoria compartilhada, este indice e aquele usado para
* 		identificar qual caractere deve ser exibido em seguida. Sem
*		semaforos, todos os processos acessam esse indice concorrentemente
*		causando conflitos. Com semaforos, o string de caracteres e exibido
*		de maneira correta (caracteres do alfabeto na ordem correta e apenas
*		um de cada caractere).
*
*		A protecao opcional com semaforo pode ser compilada no programa
*		usando a definicao de MACRO denominada PROTECT. Para compilar a
*		protecao com semaforo, retire o comentario do #define que segue.
*
*
*******************************************************************************/

/*
 * Includes Necessarios
 */
#include <errno.h>              /* errno and error codes */
#include <sys/time.h>           /* for gettimeofday() */
#include <stdio.h>              /* for printf() */
#include <unistd.h>             /* for fork() */
#include <sys/types.h>          /* for wait() */
#include <sys/wait.h>           /* for wait() */
#include <signal.h>             /* for kill(), sigsuspend(), others */
#include <sys/ipc.h>            /* for all IPC function calls */
#include <sys/shm.h>            /* for shmget(), shmat(), shmctl() */
#include <sys/sem.h>            /* for semget(), semop(), semctl() */
#include <stdlib.h>             /*ERRO N1: NAO TINHA STDLIB PARA FUNCAO EXIT()*/
#include <string.h>


/*
 * Constantes Necessarias
 */
// #define SEM_KEY_PROD		0x1243
// #define SEM_KEY_CONS		0x1086
#define SHM_BUFFERSP_KEY		0x1332
#define SHM_KEY_CONS		    0x1400
#define SHM_KEY_PROD	     	0x1432
#define SHM_BUFFER_KEY      0x1496
#define NO_OF_CHILDREN	    8


/*
 * As seguintes variaveis globais contem informacao importante. A variavel
 * g_sem_id e g_shm_id contem as identificacoes IPC para o semaforo e para
 * o segmento de memoria compartilhada que sao usados pelo programa. A variavel
 * g_shm_addr e um ponteiro inteiro que aponta para o segmento de memoria
 * compartilhada que contera o indice inteiro da matriz de caracteres que contem
 * o alfabeto que sera exibido.
*/
//IDs
  //semaforos
int	g_sem_id_prod;
int	g_sem_id_cons;
int g_sem_id_buffer;
  //memoria compartilhada
int	g_shm_id_prod;
int	g_shm_id_cons;
int g_shm_buffer_id;
int g_shm_buffersp_id;
//Addresses
int	*g_shm_addr_prod;
int *g_shm_addr_cons;
int *g_shm_buffersp;
char *g_shm_buffer;


/*
 * As seguintes duas estruturas contem a informacao necessaria para controlar
 * semaforos em relacao a "fecharem", se nao permitem acesso, ou
 * "abrirem", se permitirem acesso. As estruturas sao incializadas ao inicio
 * do programa principal e usadas na rotina PrintAlphabet(). Como elas sao
 * inicializadas no programa principal, antes da criacao dos processos filhos,
 * elas podem ser usadas nesses processos sem a necessidade de nova associacao
 * ou mudancas.
*/
struct sembuf	g_sem_op1[1];
struct sembuf	g_sem_op2[1];

/*
 * O seguinte vetor de caracteres contem o alfabeto que constituira o string
 * que sera exibido.
*/
char g_letters_and_numbers[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 1234567890\0";

/*
 * Funcoes
 */
void produzir();
void consumir();

/*
 * Programa Principal
 */
int main( int argc, char *argv[] )
{
      /*
       * Variaveis necessarias
       */
      pid_t rtn;
      int count;
      // Keys
      key_t key_sem_prod;
      key_t key_sem_cons;
      key_t key_sem_buffer;

      /*
       * Para armazenar os ids dos processos filhos, permitindo o posterior
       * uso do comando kill
       */
      int pid[NO_OF_CHILDREN];

	/*
	 * Construindo a estrutura de controle do semaforo
	 */
  // Trava
	g_sem_op1[0].sem_num   =  0;
	g_sem_op1[0].sem_op    = -1;
	g_sem_op1[0].sem_flg   =  0;
  // Libera
	g_sem_op2[0].sem_num =  0;
	g_sem_op2[0].sem_op  =  1;
	g_sem_op2[0].sem_flg =  0;


  //Criando chaves
  if ((key_sem_prod = ftok("/tmp", 'a')) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }
  if ((key_sem_cons = ftok("/tmp", 'b')) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }
  if ((key_sem_buffer = ftok("/tmp", 'c')) == (key_t) -1) {
    perror("IPC error: ftok"); exit(1);
  }
	/*
	 * Criando o semaforo
	 */
  if( ( g_sem_id_prod = semget( key_sem_prod, 1, IPC_CREAT | 0666 ) ) == -1 ) {
		fprintf(stderr,"chamada a semget() falhou, impossivel criar o conjunto de semaforos!");
		exit(1);
	}
  if( ( g_sem_id_cons = semget( key_sem_cons, 1, IPC_CREAT | 0666 ) ) == -1 ) {
		fprintf(stderr,"chamada a semget() falhou, impossivel criar o conjunto de semaforos!");
		exit(1);
	}
  if( ( g_sem_id_buffer = semget( key_sem_buffer, 1, IPC_CREAT | 0666 ) ) == -1 ) {
		fprintf(stderr,"chamada a semget() falhou, impossivel criar o conjunto de semaforos!");
		exit(1);
	}
  // Testes de trava e abertura dos semaforos
  if( semop( g_sem_id_prod, g_sem_op2, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}

	if( semop( g_sem_id_prod, g_sem_op1, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}
  if( semop( g_sem_id_cons, g_sem_op2, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}

	if( semop( g_sem_id_cons, g_sem_op1, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}
  if( semop( g_sem_id_buffer, g_sem_op2, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}

	if( semop( g_sem_id_buffer, g_sem_op1, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}
	/*
	 * Criando o segmento de memoria compartilhada
	 */
  // INT - Produtor
	if( (g_shm_id_prod = shmget( SHM_KEY_PROD, sizeof(int), IPC_CREAT | 0000)) == -1 ) {
		fprintf(stderr,"Impossivel criar o segmento de memoria compartilhada!\n");
		exit(1);
	}
	if( (g_shm_addr_prod = (int *)shmat(g_shm_id_prod, NULL, 0)) == (int *)-1 ) {
		fprintf(stderr,"Impossivel associar o segmento de memoria compartilhada!\n");
		exit(1);
	}
  // INT - Consumidor
	if( (g_shm_id_cons = shmget( SHM_KEY_CONS, sizeof(int), IPC_CREAT | 0000)) == -1 ) {
		fprintf(stderr,"Impossivel criar o segmento de memoria compartilhada!\n");
		exit(1);
	}
	if( (g_shm_addr_cons = (int *)shmat(g_shm_id_cons, NULL, 0)) == (int *)-1 ) {
		fprintf(stderr,"Impossivel associar o segmento de memoria compartilhada!\n");
		exit(1);
	}
  // BUFFER
  if( (g_shm_buffer_id = shmget( SHM_BUFFER_KEY, 3*strlen(g_letters_and_numbers)*sizeof(char), IPC_CREAT | 0000)) == -1 ) {
		fprintf(stderr,"Impossivel criar o buffer!\n");
		exit(1);
	}
	if( (g_shm_buffer = (char *)shmat(g_shm_buffer_id, NULL, 0)) == (char *)-1 ) {
		fprintf(stderr,"Impossivel associar o buffer!\n");
		exit(1);
	}
  //SP
  if( (g_shm_buffersp_id = shmget( SHM_BUFFERSP_KEY, sizeof(int), IPC_CREAT | 0000)) == -1 ) {
		fprintf(stderr,"Impossivel criar o buffer!\n");
		exit(1);
	}
	if( (g_shm_buffersp = (int *)shmat(g_shm_buffersp_id, NULL, 0)) == (int *)-1 ) {
		fprintf(stderr,"Impossivel associar o buffer!\n");
		exit(1);
	}
  // INICIALIZANDO
	*g_shm_addr_prod = 0;
  *g_shm_addr_cons = 0;
  *g_shm_buffersp = 0;


  if( semop( g_sem_id_prod, g_sem_op2, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}
  if( semop( g_sem_id_cons, g_sem_op2, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}
  if( semop( g_sem_id_buffer, g_sem_op2, 1 ) == -1 ) {
  	fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}
	/*
	 * Pergunta 3: Para que serve essa inicializa��o da mem�ria compartilhada com zero?
	 */

       /*
        * Criando os filhos
        */
       rtn = (int)1;
       for( count = 0; count < NO_OF_CHILDREN; count++ ) {
               if( (int)rtn != 0 ) {
                       pid[count] = rtn = fork();
               } else {
                       break;
                       //ERRO N2: era exit;
               }
       }

       /*
        * Verificando o valor retornado para determinar se o processo e
        * pai ou filho
        */
       if( rtn == 0 ) {
                /*
                 * Eu sou um filho
                 */
                printf("Filho %i comecou ...\n", count);

                if(count % 2 == 0){
                  produzir();
                  //printf("%s\n", "Produzir");
                }
                else{
                  consumir();
                  //printf("%s\n", "Consumir");
                }

				        //PrintChars();


        } else {
                sleep(1);

                /*
                 * Matando os filhos
                 */
                for(int i = 0; i<NO_OF_CHILDREN;i++){
                  kill(pid[i], SIGKILL);
                }

                /*
                 * Removendo a memoria compartilhada
                 */
                if( shmctl(g_shm_id_prod,IPC_RMID,NULL) != 0 ) {
                        fprintf(stderr,"Impossivel remover o segmento de memoria compartilhada (Produtor)!\n");
                        exit(1);
                }
                if( shmctl(g_shm_id_cons,IPC_RMID,NULL) != 0 ) {
                        fprintf(stderr,"Impossivel remover o segmento de memoria compartilhada (Consumidor)!\n");
                        exit(1);
                }
                if( shmctl(g_shm_buffer_id,IPC_RMID,NULL) != 0 ) {
                        fprintf(stderr,"Impossivel remover o segmento de memoria compartilhada (Buffer)!\n");
                        exit(1);
                }

                /*
                 * Removendo o semaforo
                 */
                if( semctl( g_sem_id_prod, 0, IPC_RMID, 0) != 0 ) {
                        fprintf(stderr,"Impossivel remover o conjunto de semaforos (Produtor)!\n");
                        exit(1);
                }
                if( semctl( g_sem_id_cons, 0, IPC_RMID, 0) != 0 ) {
                        fprintf(stderr,"Impossivel remover o conjunto de semaforos (Consumidor)!\n");
                        exit(1);
                }
                if( semctl( g_sem_id_buffer, 0, IPC_RMID, 0) != 0 ) {
                        fprintf(stderr,"Impossivel remover o conjunto de semaforos (Buffer)!\n");
                        exit(1);
                }

                printf("\n");
                exit(0);
        }
}

void produzir(){
  struct timeval tv;
  int number;

	int tmp_index;
	int i;

	usleep(100000);

  while(1) {

  		/*
                   * Conseguindo o tempo corrente, os microsegundos desse tempo
               	 * sao usados como um numero pseudo-randomico. Em seguida,
              	 * calcula o numero randomico atraves de um algoritmo simples
  		 */
  		if( gettimeofday( &tv, NULL ) == -1 ) {
  			fprintf(stderr,"Impossivel conseguir o tempo atual, terminando.\n");
  			exit(1);
  		}
      number = ((tv.tv_usec / 47) % 5) + 1; //até 5
  // INICIO REGIAO CRITICA
  #ifdef PROTECT
  		if( semop( g_sem_id_prod, g_sem_op1, 1 ) == -1 ) {
         	fprintf(stderr,"chamada semop() falhou, impossivel fechar o recurso!");
         	exit(1);
      }
      if( semop( g_sem_id_buffer, g_sem_op1, 1 ) == -1 ) {
          fprintf(stderr,"chamada semop() falhou, impossivel fechar o recurso!");
          exit(1);
      }
  #endif

  		tmp_index = *g_shm_addr_prod;
      int sp = *g_shm_buffersp;
      printf("Adicionado = ");
      for(i = 0; i < number; i++){
        if(g_letters_and_numbers[tmp_index + i] == '\0'){ // Acabou ABC
          if((sp + i) == (3*(tmp_index+i+1))-1){ // Lotou Buffer
            g_shm_buffer[sp + i] = '\0';
            printf("\n");
            printf("PROD=\n%s\n\n", g_shm_buffer);
            *g_shm_buffersp = 0;
          }
          else{ // Proxima linha
            g_shm_buffer[sp + i] = '\n';
            *g_shm_buffersp = sp + i + 1;
          }
          *g_shm_addr_prod = 0;
          break;
        }
        // Normal
        char letra = g_letters_and_numbers[tmp_index + i];
        g_shm_buffer[sp + i] = letra;
        printf("%c", letra);
      }
      if(i == number){
        printf("\n");
        *g_shm_buffersp = sp + i;
        *g_shm_addr_prod = tmp_index + i;
      }
      /*
  		 * Liberando o recurso se a macro PROTECT estiver definida
  		 */

  #ifdef PROTECT //ERRO: ESTAVA ABRINDO NAO FECHANDO
  		if( semop( g_sem_id_prod, g_sem_op2, 1 ) == -1 ) {
        fprintf(stderr,"chamada semop() falhou, impossivel liberar o recurso!");
       	exit(1);
     	}
      if( semop( g_sem_id_buffer, g_sem_op2, 1 ) == -1 ) {
        fprintf(stderr,"chamada semop() falhou, impossivel liberar o recurso!");
       	exit(1);
     	}
  #endif
    //FIM REGIAO CRITICA
  }
}

void consumir(){
  struct timeval tv;
  int number;

	int tmp_index;
	int i;

	usleep(100000);
  while(1) {

  		/*
                   * Conseguindo o tempo corrente, os microsegundos desse tempo
               	 * sao usados como um numero pseudo-randomico. Em seguida,
              	 * calcula o numero randomico atraves de um algoritmo simples
  		 */
  		if( gettimeofday( &tv, NULL ) == -1 ) {
  			fprintf(stderr,"Impossivel conseguir o tempo atual, terminando.\n");
  			exit(1);
  		}
      number = ((tv.tv_usec / 47) % 5) + 1; //até 5


  // INICIO REGIAO CRITICA
  #ifdef PROTECT
  		if( semop( g_sem_id_cons, g_sem_op1, 1 ) == -1 ) {
       	fprintf(stderr,"chamada semop() falhou, impossivel fechar o recurso!");
       	exit(1);
     	 }
       if( semop( g_sem_id_buffer, g_sem_op1, 1 ) == -1 ) {
        	fprintf(stderr,"chamada semop() falhou, impossivel fechar o recurso!");
        	exit(1);
      	 }
       // if( semop( g_sem_id_prod, g_sem_op1, 1 ) == -1 ) {
       //  	fprintf(stderr,"chamada semop() falhou, impossivel fechar o recurso!");
       //  	exit(1);
      	//  }
  #endif
      // Somente para verificar para nao passar a producao;
      int prod_lim = *g_shm_addr_prod;
  // #ifdef PROTECT
  //     if( semop( g_sem_id_prod, g_sem_op2, 1 ) == -1 ) {
  //       fprintf(stderr,"chamada semop() falhou, impossivel liberar o recurso!");
  //       exit(1);
  //     }
  // #endif
  		tmp_index = *g_shm_addr_cons;
      int end_buffer = 0;
  		for( i = 0; i < number; i++ ) {
        //printf("%d\n", number);
        if(tmp_index + i == prod_lim) break;
        if(g_shm_buffer[tmp_index+i] == '\0'){
          end_buffer = 1;
          break;
        }
        if(g_shm_buffer[tmp_index + i] != '\n'){
          g_shm_buffer[tmp_index + i] = '#';
        }
				usleep(number);
  		}
  		tmp_index += i;
      if(end_buffer){
        printf("CONS=\n%s\n\n",g_shm_buffer);
        tmp_index = 0;
      }
      *g_shm_addr_cons = tmp_index;
  		/*
  		 * Liberando o recurso se a macro PROTECT estiver definida
  		 */
  #ifdef PROTECT //ERRO: ESTAVA ABRINDO NAO FECHANDO
  		if( semop( g_sem_id_cons, g_sem_op2, 1 ) == -1 ) {
        fprintf(stderr,"chamada semop() falhou, impossivel liberar o recurso!");
       	exit(1);
     	}
      if( semop( g_sem_id_buffer, g_sem_op2, 1 ) == -1 ) {
        fprintf(stderr,"chamada semop() falhou, impossivel liberar o recurso!");
       	exit(1);
     	}
  #endif
    //FIM REGIAO CRITICA
  }
}
