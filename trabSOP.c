#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "source/lista.h"

#define MIN(a,b) ((a) < (b) ? a : b)
#define TAMNOMEMAX 100

int exitFlag;
int threadsOcupadas;
pthread_mutex_t lock;

//Estrutura da oferta de compra e venda
typedef struct {
    int  quantidade;
    int  regQuantidade;
    char nome[30];
} info;

//Estrutura dos argumentos passados aos corretores
struct argumentos_struct {
    int  * nroThread;
    char * nomeArq;
};
typedef struct argumentos_struct argThread;
argThread * newArgThread(int nroThread, char * nomeArq) {
    argThread * aux;
    aux = (argThread *) malloc(sizeof(argThread));
    aux -> nroThread  = (int *) malloc(sizeof(int));
    *(aux -> nroThread)  = nroThread;

    aux -> nomeArq    = malloc(sizeof(char) * strlen(nomeArq));
    memcpy(aux->nomeArq, nomeArq, sizeof(char) * strlen(nomeArq));
    return aux;
}

char * nomeArquivo;
LDDE * vendedor;

/*
    buscaOfertaCompra
    Funcao que realiza compras
    Tem como entrada
    - ponteiro da lista de ofertas do comprador
    - nome do produto que esta a venda
    - quantidade disponivel para venda
*/

int quantidadeNode(NoLDDE * tmpNo) {
    return (*(info *)tmpNo->dados).quantidade;
}

char * nomeNode(NoLDDE * tmpNo) {
    return (*(info *)tmpNo->dados).nome;
}

int buscaOfertaCompra(LDDE *p, char nome[], int quantDispVendedor){
    NoLDDE *aux;
    aux = p->inicioLista;
    info *reg, x;
    reg = &x;
    int quantidadeComprada;
    //Percorre lista comprador
    while(aux->prox!=NULL) {
        //obtem cada oferta do comprador
        memcpy(reg, aux->dados, p->tamInfo);
        //se quantidade é zero então o comprador está satisfeito
        if(reg->quantidade == 0){
            //printf("satifeito\n");
            return 0;
        }
        //Compara o nome do produto da lista do vendedor com o nome da lista
        // do comprador
        if(memcmp(reg->nome,nome,sizeof(reg->nome))){
            //Se iguais quer dizer que o comprador quer comprar algo com aquele nome
            printf("encontrado\n");
            //Se o vendedor tem uma quantidade maior que o comprador deseja
            //o comprador compra todas daquele produto
            if(quantDispVendedor >= reg->quantidade){
                //comprador fica satisfeito em relação aquele produto
                quantidadeComprada = reg->quantidade;
                reg->quantidade = 0;
                //registra na lista do vendedor o valor atualizado
                memcpy(aux->dados, reg, p->tamInfo);
                //retorna a quatidade comprada
                return quantidadeComprada;
            }else{
                //Se o vendedor tem menos que o comprador precisa
                reg->quantidade = reg->quantidade - quantDispVendedor;
                memcpy(aux->dados, reg, p->tamInfo);
                quantidadeComprada = quantDispVendedor;
                return quantidadeComprada;
            }

        }
        aux = aux->prox;
    }
  return 0;
}


int quantidadeCompra(NoLDDE * ptrVendedor, NoLDDE * ptrOferta) {
    int qtdVenda  = quantidadeNode(ptrVendedor);
    int qtdCompra = quantidadeNode(ptrOferta);

    return MIN(qtdVenda, qtdCompra);
}


void * corretor(void *argumentos){
	LDDE * listaOfertas = listaCriar(sizeof(info));
    int quantidadeComprada;
	argThread * args = (argThread *) argumentos;

	if(listaOfertas != NULL) {
        //Leitura do arquivo do corretor
        info *reg = (info *) malloc(sizeof(reg));

        FILE *ptr = fopen(args->nomeArq, "r");
        if(ptr){
        	while((fscanf(ptr,"%s %d\n", reg->nome, &reg->quantidade)) != EOF){
        		reg->regQuantidade = reg->quantidade;
				listaInserir(listaOfertas, reg);
        	}

        	fclose(ptr);
        }
        //local de disputa pela variavel thread ocupadas
        pthread_mutex_lock(&lock);
        threadsOcupadas = threadsOcupadas-1;
        printf("[%s] leu arquivo\n", args->nomeArq);
        NoLDDE * temp = listaOfertas->inicioLista;
        pthread_mutex_unlock(&lock);

        //local de verificar ofertas de vendas e realizar a compra
        //Codigo apenas para testes
        NoLDDE *ptrOferta;
        NoLDDE *ptrVendedor;
        //Thread Espera vendedor popular lista
        while(1) {
            ptrVendedor = vendedor->inicioLista;
            if(ptrVendedor != NULL) break;
        }

        printf("[%s] PtrVendedor deixou de ser NULL\n", args->nomeArq);

        while(1) {
            ptrOferta = listaOfertas->inicioLista;
            while(ptrOferta != NULL) {
                if(strcmp(nomeNode(ptrVendedor), nomeNode(ptrOferta)) == 0) {
                    pthread_mutex_lock(&lock);
                    if(quantidadeNode(ptrVendedor) > 0 && quantidadeNode(ptrOferta) > 0) {
                        int qtdCompra = quantidadeCompra(ptrVendedor, ptrOferta);
                        printf("[%s] Comprando %d de %s\n", args->nomeArq, qtdCompra, nomeNode(ptrVendedor));
                        (*(info *)ptrVendedor->dados).quantidade -= qtdCompra;
                        (*(info *)ptrOferta->dados).quantidade   -= qtdCompra;
                    }
                    pthread_mutex_unlock(&lock);
                }
                ptrOferta = ptrOferta->prox;
            }

            if(ptrVendedor == vendedor->fimLista && exitFlag) { //ponteiro chegou ao fim da lista e não há mais itens para serem inseridos
                printf("[%s] ptrVendedor chegou ao fim\n", args->nomeArq);
                pthread_exit(NULL);
            } else if(ptrVendedor == vendedor->fimLista && !exitFlag) //ponteiro cheogu ao fim da lista e ainda há itens a serem inseridos
                continue;
            else { //ponteiro não chegou ao fim da lista então ele pode avançar
                ptrVendedor = ptrVendedor->prox;
            }
        }
    	destroi(&listaOfertas);
    }else{
    	printf("errro ao criar lista\n");
    }

	pthread_exit(NULL);
}

void imprimirPort(LDDE *lista){
	NoLDDE * temp1 = lista->inicioLista;
	NoLDDE * temp2 = lista->inicioLista;
	int demanda = 0;
	int compra = 0;

	while(temp1!=NULL){
		temp1 = temp1->prox;
		temp2 = lista->inicioLista;
		demanda = (*(info *)temp2->dados).regQuantidade;
		while(temp2!=NULL){
			if((strcmp(nomeNode(temp1), nomeNode(temp2)) == 0)){
				compra +=  demanda - (*(info *)temp2->dados).quantidade;
			}
			temp2 = temp2->prox;
		}
		//printf("print aqui nome, demanda, compra\n", );
	}
}


int main(int argc, char** argv) {
    system("tput reset");
    exitFlag = 0;
	int qtdThreads; //Quantidade de threads
	int i;
    vendedor = listaCriar(sizeof(info));

    if(pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "Erro na criação do Mutex, amigo se chegou aqui você é um campeão");
        exit(EXIT_FAILURE);
    }

	//Argumentos passados pela linha de comando
    qtdThreads = atoi(argv[1]);
    threadsOcupadas = qtdThreads;
    pthread_t threadCorretor[qtdThreads];

	//Cria threads corretores
	for(i = 0; i < qtdThreads; i++){
        char nomeArq[20];
        snprintf(nomeArq, sizeof(nomeArq), "%s-%d", argv[2], i+1);
        argThread * args = newArgThread(i, nomeArq);
		pthread_create(&threadCorretor[i], NULL, corretor, (void *) args);
    }

    while(1) {
        if(threadsOcupadas == 0)
            break;
    }

    printf("[ Main ] Todos os arquivos foram lidos\n\n");
	FILE * ptr = fopen(argv[2],"r");
	if(vendedor != NULL) {
        if(ptr){
        	info * reg = (info *) malloc(sizeof(reg));
        	while((fscanf(ptr,"%s %d\n", reg->nome, &reg->quantidade)) != EOF){
        		if(reg->nome[0] == '#'){
        			//usleep(reg->quantidade*1000);
                    sleep(1);
        		}else{
                    listaInserir(vendedor, reg);
                    printf("[ Main ] Inserindo: %s %d\n", nomeNode(vendedor->fimLista), quantidadeNode(vendedor->fimLista));
        		}
        	}
        	fclose(ptr);
        }
    }
    printf("[ Main ] Fim inserção lista vendedor\n");
    exitFlag = 1;

    for(i = 0; i<qtdThreads; i++){
		pthread_join(threadCorretor[i], NULL);
	}

	destroi(&vendedor);
    return 0;
}
