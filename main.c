#include <stdio.h>
#include <openssl/bn.h>
#include <pthread.h>

struct _matrix {
	int level;	//current level
	int goal;	//goal level
	int id;	//thread id
	struct _matrix * other; //pointer to another thread;
	
	BIGNUM * f00;
	BIGNUM * f01;
	BIGNUM * f10;
	BIGNUM * f11;
	
	BIGNUM ** fextern;	//extern array
};
typedef struct _matrix matrix;

matrix * matrix_alloc(int i, BIGNUM ** numbers){
	matrix * m = malloc(sizeof(matrix));
	m->fextern=numbers;
	m->level=i;
	
	if (i<=0) {
		m->level=0;
		m->f00=numbers[1];
		m->f01=numbers[1];
		m->f10=numbers[1];
		m->f11=numbers[0];
		return m;
	}
	if (numbers[i+1] && numbers[i]) {
		m->f00=numbers[i+1];
		m->f01=m->f10=numbers[i];
		numbers[i-1]=BN_new();
		BN_sub(numbers[i-1], m->f00, m->f10);
		m->f11=numbers[i-1];
		return m;
	}else if (numbers[i] && numbers[i-1]) {
		m->f01=m->f10=numbers[i];
		m->f11=numbers[i-1];
		numbers[i+1]=BN_new();
		BN_add(numbers[i+1], m->f01, m->f11);
		m->f00=numbers[i+1];
		return m;
	}
	printf("init empty matrix");
	return m;
}

void print_matrix(matrix * m){
	//printf("level %d\n",m->level);
	printf("number %d-%d\n",m->level-1,m->level+1);
	printf("%s\t%s\n%s\t%s\n",BN_bn2dec(m->f00),BN_bn2dec(m->f10),BN_bn2dec(m->f01),BN_bn2dec(m->f11));
	return;
}

void * runner(void *arg){
	matrix * m = (matrix *)(arg);
	//printf("thread %d says\n",m->id);
	//print_matrix(m);
	//print_matrix(m->other);
	BN_CTX * ctx=BN_CTX_new();
	
	while (m->level+m->other->level < m->goal) {
		// m=m*m, update members, m->level
		if (m->id==0) {
			printf("thread %d,level %d\n",m->id,m->level);
			print_matrix(m);
		}
		BIGNUM * tmp_n1_n1=BN_new();
		BIGNUM * tmp_n1_n=BN_new();
		BIGNUM * tmp_n_n0=BN_new();
		BIGNUM * tmp_n_n=BN_new();
		BIGNUM * tmp_n0_n0=BN_new();
		
		BN_mul(tmp_n1_n1, m->f00, m->f00, ctx);
		BN_mul(tmp_n1_n, m->f00, m->f10, ctx);
		BN_mul(tmp_n_n0, m->f10, m->f11, ctx);
		BN_mul(tmp_n_n, m->f10, m->f01, ctx);
		BN_mul(tmp_n0_n0, m->f11, m->f11, ctx);
		
		if (!m->fextern[m->level*2]) {
			m->fextern[m->level*2]=BN_new();
		}
		if (!m->fextern[m->level*2+1]) {
			m->fextern[m->level*2+1]=BN_new();
		}
		if (!m->fextern[m->level*2-1]) {
			m->fextern[m->level*2-1]=BN_new();
		}
		
		BN_add(m->fextern[m->level*2], tmp_n1_n, tmp_n_n0);
		//BN_add(m->fextern[m->level*2+1], tmp_n1_n1, tmp_n1_n);
		BN_add(m->fextern[m->level*2-1], tmp_n_n, tmp_n0_n0);
		BN_add(m->fextern[m->level*2+1],m->fextern[m->level*2-1],m->fextern[m->level*2]);
		
		m->f00=m->fextern[m->level*2+1];
		m->f10=m->f01=m->fextern[m->level*2];
		m->f11=m->fextern[m->level*2-1];
		m->level=m->level*2;
		
		pthread_yield_np();

		
		BN_free(tmp_n1_n1);
		BN_free(tmp_n1_n);
		BN_free(tmp_n_n0);
		BN_free(tmp_n_n);
		BN_free(tmp_n0_n0);
	}
	
	printf("thread %d end with level %d\n",m->id,m->level);
	//print_matrix(m);
	return NULL;
}

int main (int argc, const char * argv[]) {
    // insert code here...
    if (argc != 2) {
		printf("Usage: %s d\n",argv[0]);
		printf("d: generate d(th) fib number\n");
		return -1;
	}
	int num=atoi(argv[1]);
	printf("generate %dth fib number\n",num);
	
	BIGNUM ** numbers=malloc(sizeof(BIGNUM *)*num*2);
	
	
	BN_dec2bn(&numbers[0],"0");
	BN_dec2bn(&numbers[1],"1");
	BN_dec2bn(&numbers[2],"1");
	BN_dec2bn(&numbers[3],"2");
	BN_dec2bn(&numbers[4],"3");
	BN_dec2bn(&numbers[5],"5");
	BN_dec2bn(&numbers[6],"8");
	BN_dec2bn(&numbers[7],"13");
	
	matrix * m2=matrix_alloc(2, numbers);
	matrix * m5=matrix_alloc(5, numbers);
	m2->id=0;
	m5->id=1;
	m2->goal=m5->goal=num;
	
	matrix * m[2];
	m[0]=m2;
	m[1]=m5;
	m2->other=m5;
	m5->other=m2;
	
	pthread_t th0;
	pthread_t th1;
	
	pthread_create(&th0, NULL, &runner, m2);
	pthread_create(&th1, NULL, &runner, m5);
	
	pthread_join(th0, NULL);
	pthread_join(th1, NULL);
	
	print_matrix(m2);
	print_matrix(m5);
	
	printf("dumping list of generated numbers\n");
	for (int i=0; i<num*2; i++) {
		if (numbers[i]) {
			printf("%d %s\n",i,BN_bn2dec(numbers[i]));
		}else {
			printf("%d\n",i);
		}

	}
    return 0;
}

