#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <ctype.h>
#include "dbg.h"

#define TRUE 1
#define FALSE 0

#define minim(a,b) ((a<b)?a:b)
#define maxim(a,b) ((a>b)?a:b)

void		read_var  (char *line, char *result);
int            *ivector(int nl, int nh);
char           *cvector(int nl, int nh);
unsigned char  *ucvector(int nl, int nh);
unsigned long  *ulvector(int nl, int nh);
unsigned int   *uvector(int nl, int nh);
double         *dvector(int nl, int nh);
double        **dmatrix(int nrl, int nrh, int ncl, int nch);
char          **cmatrix(int nrl, int nrh, int ncl, int nch);
unsigned char **ucmatrix(int nrl, int nrh, int ncl, int nch);
unsigned long **ulmatrix(int nrl, int nrh, int ncl, int nch);
unsigned int  **umatrix(int nrl, int nrh, int ncl, int nch);
double		ran1    (int *idum);
double		gamdev  (int ia, int *idum);
double		gammln  (double xx);
double		poidev  (double xm, int *idum);
double		gs      (double alpha);
double		gcf     (double alpha);
double		rgamma  (double alpha);
double		rbeta   (double u, double v);
int		pick       (int n);
void		free_cvector(char *v, int nl, int nh);
void		free_ucvector(unsigned char *v, int nl, int nh);
void		free_ulvector(unsigned long *v, int nl, int nh);
void		free_uvector(unsigned int *v, int nl, int nh);
void		free_dvector(double *v, int nl, int nh);
void		free_dmatrix(double **m, int nrl, int nrh, int ncl, int nch);
void		free_cmatrix(char **m, int nrl, int nrh, int ncl, int nch);
void		free_ucmatrix(unsigned char **m, int nrl, int nrh, int ncl, int nch);
void		free_ulmatrix(unsigned long **m, int nrl, int nrh, int ncl, int nch);
int           **imatrix(int nrl, int nrh, int ncl, int nch);
void		free_imatrix(int **m, int nrl, int nrh, int ncl, int nch);
void		set_next  (char *sss, int *i, int *j);
void		flat_index_contig(int *index, char *contig, int L, int depth);
int 
tile_contig(int *flat, char *contig, int L, int window, int primer,
	    int ampli, int depth, int local_depth, int start_pos);
void		count_copys(int *flat, char *s, int n, int *reps);
void		reverse_transcribe(char *contig, char *s, int n);
void		reverse_string(char *contig, char *s, int n);
int		sort_compare_index(const void *a, const void *b);
int		check_hairpin(char *ss, int n);
int		check_dimer(char *p1, char *p2, int n);
int		check_watson_crick(char a, char b);
int		test_dimer (char *p1, char *p2, int n);
unsigned int	encode_basepairs(char *ss, int n);
void		decode_basepairs(unsigned char *s, char *dest, int n);
void		index_string(int *index, char *s, int n);
void		convert_int_basepairs(int i, char *s, int k);
void 
fill_quality_scores(int *flat, int *local, char *contig, int L, int window, int depth, int ld, int primer,
		    double *fq_left, double *gc_left, int *index_left, double min_gc, double max_gc);
int		increase_buffer(int n);

static FILE    *outfile, *innfile;
static unsigned int *DUMP_LIST;
static int	idum;



int 
main()
{
	char          **filename, ss[4196], sss[4196], basename[1024];
	char		scratch_pad, **contig_descript;
	int		i         , k, N, not_done, max_mers;
	unsigned int	j;
	int		fasta     , idepth, total_index;
	int		max_index;
	int		min_index;
	long long	tot_mers;
	int		length_from_N;
	unsigned int	maskit;
	unsigned int   *contig_length, max_sites, gpos, *this_mer, s1_mer;
	unsigned int  **flat_index, newpos, expos, realpos;
	unsigned int   *flat_index_count;
	unsigned int   *flat_index_max;

	FILE           *sfile, *mfile, *seqfile;

	outfile = stdout;
	read_var("\nSend Output to Screen or Disk? [S,D]\n", ss);

	if ((strchr(ss, 'D')) || (strchr(ss, 'd')))
	{
		read_var("Please Enter File Name for Output\n", ss);
		if ((outfile = fopen(ss, "a")) == (FILE *) NULL)
		{
			printf("\n Can not open file %s\n", ss);
			exit(1);
		}
	} else
		outfile = stdout;

	read_var("Maximum Number of Contig Fasta Files to Process\n", ss);
	N = atoi(ss);
	filename = cmatrix(0, N, 0, 256);

	contig_descript = cmatrix(0, N, 0, 4196);
	contig_length = uvector(0, N);


	sprintf(sss, "Please Enter Name For Fastaq File\n");
	read_var(sss, filename[0]);
	if ((innfile = fopen(filename[0], "r")) == (FILE *) NULL)
	{
		printf("\n Can not open file %s\n", filename[0]);
		exit(1);
	}
	read_var("Basename to save compressed Genome and Indexes\n", basename);

	read_var("Maximum Number Of Sites in all contigs\n", ss);
	max_sites = (unsigned)atol(ss);

	read_var("Index Depth\n", ss);
	idepth = atoi(ss);
	sprintf(sss, "%s.sdx", basename);
	if ((sfile = fopen(sss, "w")) == (FILE *) NULL)
	{
		printf("\nCould Not Open file %s\n", sss);
		exit(1);
	}
	sprintf(ss, "%s.seq", basename);
	if ((seqfile = fopen(ss, "w")) == (FILE *) NULL)
	{
		printf("\nCould Not Open file %s\n", ss);
		exit(1);
	}
	not_done = TRUE;

	expos = 0;
	realpos = 0;
	total_index = (int)floor(pow(4, idepth));
	tot_mers = (long long)(3 * idepth + 1) * max_sites;
	min_index = 0;
	maskit = (unsigned int)(total_index - 1);
	max_index = maxim(min_index, (unsigned long)6.4e9 / (sizeof(unsigned int) * (minim(total_index, tot_mers))));
	printf("\n Determined the size of flat index to be %u * %u = %u bytes with max_index = %u and total elements is %lld \n",
	       total_index, (int)sizeof(unsigned int), (int)(total_index * sizeof(unsigned int)), max_index, tot_mers);


	flat_index = (unsigned int **)malloc((unsigned)(total_index + 1) * sizeof(unsigned *));
	flat_index_count = uvector(0, total_index);
	flat_index_max = uvector(0, total_index);

	printf("\n Just allocated flat index \n");

	gpos = 0;
	fgets(sss, 4195, innfile);
	not_done = TRUE;
	newpos = 0;
	fasta = -1;

	for (i = 0; i <= total_index; i++)
	{
		flat_index_count[i] = 0;
		flat_index_max[i] = min_index;
	}


	max_mers = 4 * idepth;
	this_mer = uvector(0, max_mers);
	length_from_N = 0;
	s1_mer = 0;
	while ((not_done) && (gpos + newpos < max_sites))
	{
		if (sss[0] == '>')
		{
			if (fasta % 500 == 0)
				printf("\n Working on %s which is fasta = %d\n\n", sss, fasta);
			contig_length[fasta] = newpos;
			gpos += newpos;
			newpos = 1 - idepth;
			s1_mer = 0;
			length_from_N = 0;
			for (i = 0; i <= max_mers; i++)
				this_mer[i] = 0;

			fasta++;
			/*
			 * sprintf(sss,"%s.cdx",filename[fasta]); if((cfile=fopen(sss,"w"))==(FILE *)NULL) {
			 * printf("\nCould Not Open file %s\n",sss); exit(1); }
			 */

			j = strlen(sss);
			while (j > 1 && !isalnum(sss[j]))
			{
				sss[j] = '\0';
				j--;
			}

			for (i = 1; i <= j; i++)
				if (!isspace(sss[i]))
					contig_descript[fasta][i - 1] = sss[i];
				else
					contig_descript[fasta][i - 1] = '_';
			contig_descript[fasta][j] = '\0';
			//printf("\n j = %d Contig description is %s \n", j, contig_descript[fasta]);
			fgets(sss, 256, innfile);
		}
		/* printf("\n %s \n",sss); */
		j = strlen(sss);

		for (i = 0; i < j; i++)
		{
			if (isalpha(sss[i]))
			{
				scratch_pad = toupper(sss[i]);
				fputc(scratch_pad, seqfile);

				if (scratch_pad == 'N')
				{
					length_from_N = 0;
					for (k = 0; k <= max_mers; k++)
						this_mer[k] = 0;
				} else
				{
					unsigned	bit = 0;
					if (scratch_pad == 'C')
						bit = 1;
					else if (scratch_pad == 'G')
						bit = 2;
					else if (scratch_pad == 'T')
						bit = 3;

					s1_mer = s1_mer << 2;
					s1_mer &= maskit;

					for (k = max_mers - 1; k >= 0; k--)
					{
						if (k < 4)
							this_mer[k] = s1_mer + k;
						else
							this_mer[k] = (this_mer[k - 4] << 2) + bit;
						this_mer[k] &= maskit;
					}
					s1_mer += bit;

					length_from_N++;
					unsigned int	where_now = gpos + newpos;
					if (length_from_N >= idepth)
						for (k = 0; k < max_mers; k++)
							if ((k < 4) || (this_mer[k] != s1_mer))
							{
								if (flat_index_count[this_mer[k]] == flat_index_max[this_mer[k]])
								{
									int		new_size = increase_buffer(flat_index_count[this_mer[k]]);
									if (flat_index_max[this_mer[k]] == 0)
									{
										flat_index[this_mer[k]] = uvector(0, new_size - 1);
										flat_index_max[this_mer[k]] = new_size;
									} else
									{
										unsigned int   *tvect = uvector(0, new_size - 1);
										memcpy(tvect, flat_index[this_mer[k]], sizeof(unsigned int) * flat_index_count[this_mer[k]]);
										free_uvector(flat_index[this_mer[k]], 0, flat_index_max[this_mer[k]]);
										flat_index[this_mer[k]] = tvect;
										flat_index_max[this_mer[k]] = new_size;
									}
								}
								flat_index[this_mer[k]][flat_index_count[this_mer[k]]] = where_now;
								flat_index_count[this_mer[k]]++;
							}
				}
				newpos++;
			}
		}

		if (feof(innfile) != 0)
			not_done = FALSE;
		else
			not_done = TRUE;

		if (not_done)
		{
			sss[0] = '\0';
			fgets(sss, 256, innfile);
			if (strlen(sss) < 1)
				not_done = FALSE;
		}
	}

	fclose(seqfile);
	contig_length[fasta] = newpos;
	fasta++;


	sprintf(ss, "%s.mdx", basename);
	if ((mfile = fopen(ss, "w")) == (FILE *) NULL)
	{
		printf("\nCould Not Open file %s\n", ss);
		exit(1);
	}
	k = 0;

	for (i = 0; i < total_index; i++)
	{
		fwrite(&flat_index_count[i], sizeof(unsigned), 1, mfile);
		if (flat_index_count[i] > 0)
			fwrite(flat_index[i], sizeof(unsigned), flat_index_count[i], mfile);
	}
	fclose(mfile);


	if (gpos + newpos == max_sites)
		fprintf(outfile, "\nContig %s was truncated at %d sites\n", basename, max_sites);


	fprintf(sfile, "%d\n", fasta);
	for (i = 0; i < fasta; i++)
		fprintf(sfile, "%d\t%s\n", contig_length[i], contig_descript[i]);
	fprintf(sfile, "%d\n", idepth);
	fclose(sfile);
	return (0);

}

/*---------------------------------------------------------------------*/
int 
increase_buffer(int n)
{
	if (n <= 0)
		return 4;
	if (n < 100)
		return 3 * n / 2;
	if (n < 1000)
		return 6 * n / 5;
	return 11 * n / 10;
}
/*---------------------------------------------------------------------*/
void 
convert_int_basepairs(int i, char *s, int k)
{
	int		j         , l;
	char		ss        [256];

	l = k - 1;
	for (j = 0; j < 256; j++)
		ss[j] = 'A';

	while (l >= 0)
	{
		j = i % 4;
		if (j == 0)
			ss[l] = 'A';
		else if (j == 1)
			ss[l] = 'C';
		else if (j == 2)
			ss[l] = 'G';
		else
			ss[l] = 'T';
		i -= j;
		i /= 4;
		l--;
	}

	ss[k] = '\0';
	strcpy(s, ss);

}
/*---------------------------------------------------------------------*/
void 
decode_basepairs(unsigned char *s, char *dest, int n)
{
	int		i         , m, l;
	unsigned char	j;
	char		a        , ss[6];

	ss[5] = '\0';
	for (i = 0; i < n; i++)
	{
		j = s[i];

		/* printf("\n Decoding %d ",j); */
		for (l = 3; l >= 0; l--)
		{
			m = j % 4;
			if (m == 0)
				a = 'A';
			else if (m == 1)
				a = 'C';
			else if (m == 2)
				a = 'G';
			else
				a = 'T';

			ss[l] = a;
			j = j >> 2;;

		}

		for (m = 0; m < 4; m++)
			*dest++ = ss[m];

		/* printf("as %s",ss); */
	}
}
/*---------------------------------------------------------------------*/
unsigned int 
encode_basepairs(char *ss, int n)
{
	unsigned int	k;
	int		i;

	k = 0;
	for (i = 0; i < n; i++)
	{
		k = k << 2;
		if (ss[i] == 'A');
		else if (ss[i] == 'C')
			k++;
		else if (ss[i] == 'G')
			k += 2;
		else
			k += 3;
	}

	/* printf("\nn = %d Encoding %c%c%c%c as %d ",n,ss[0],ss[1],ss[2],ss[3],k);  */


	return k;
}
/*---------------------------------------------------------------------*/
int 
check_watson_crick(char a, char b)
{
	a = toupper(a);
	b = toupper(b);
	if (a == 'A')
		if (b == 'T')
			return TRUE;
		else
			return FALSE;
	else if (a == 'T')
		if (b == 'A')
			return TRUE;
		else
			return FALSE;
	else if (a == 'G')
		if (b == 'C')
			return TRUE;
		else
			return FALSE;
	else if (a == 'C')
		if (b == 'G')
			return TRUE;
		else
			return FALSE;
	else
		return FALSE;
	//This is impossible

}
/*---------------------------------------------------------------------*/
void 
count_copys(int *flat, char *s, int n, int *reps)
{
	int		i         , j;
	unsigned int	offset, k;
	char		ss        [256];


	offset = 0;
	for (i = 1; i <= n; i++)
	{
		if ((toupper(s[i - 1]) == 'N') || (toupper(ss[i - 1]) == 'N'))
		{
			for (j = 1; j <= n; j++)
				reps[j] += 100000000;
			return;
		}
		k = encode_basepairs(s, i);
		reps[i] += flat[k + offset];
		/*
		 * if(i == 1) printf("\n n = %d forward k = %d",n,k+offset);
		 */
		reverse_transcribe(s, ss, i);
		k = encode_basepairs(ss, i);
		/*
		 * if(i == 1) printf("\n reverse k = %d",k+offset);
		 */
		reps[i] += flat[k + offset];
		offset++;
		offset = offset << 2;
	}

}
/*---------------------------------------------------------------------*/
void 
reverse_string(char *contig, char *s, int n)
{
	int		i;

	for (i = n - 1; i > -1; i--)
	{
		*s++ = *(contig + i);
	}

}
/*---------------------------------------------------------------------*/
void 
reverse_transcribe(char *contig, char *s, int n)
{
	int		i;
	char		c;

	for (i = n - 1; i > -1; i--)
	{
		if (*(contig + i) == 'A')
			c = 'T';
		else if (*(contig + i) == 'C')
			c = 'G';
		else if (*(contig + i) == 'G')
			c = 'C';
		else if (*(contig + i) == 'T')
			c = 'A';
		else
			c = 'N';

		*s++ = c;
	}

}
/*---------------------------------------------------------------------*/


int 
sort_compare(const void *a, const void *b)
{
	if (*((double *)a) < *((double *)b))
		return -1;
	else if (*((double *)a) > *((double *)b))
		return 1;
	else
		return 0;
}
/*---------------------------------------------------------------------*/

int 
sort_compare_index(const void *a, const void *b)
{
	unsigned int	ad, bd;

	ad = DUMP_LIST[*((int *)a)];
	bd = DUMP_LIST[*((int *)b)];

	if (ad < bd)
		return -1;
	else if (ad > bd)
		return 1;
	else
		return 0;
}
/*---------------------------------------------------------------------*/

void 
set_next(char *sss, int *i, int *j)
{

	while (!(isalnum(sss[*i]) || (sss[*i] == '(') || (sss[*i] == '*') || (sss[*i] == '-')))
		(*i)++;
	*j = *i;
	while (!isspace(sss[*j]) && (sss[*j] != '"'))
		(*j)++;
	sss[*j] = '\0';
	/* printf("\n %s",&sss[*i]); */
}
/*---------------------------------------------------------------------*/
int 
pick(int n)
{
	return (int)floor((double)n * ran1(&idum));
}
/*---------------------------------------------------------------------*/
#define M1 259200l
#define IA1 7141l
#define IC1 54773l
#define RM1 (1.0/M1)
#define M2 134456l
#define IA2 8121l
#define IC2 28411l
#define RM2 (1.0/M2)
#define M3 243000l
#define IA3 4561l
#define IC3 51349l

double 
ran1(int *idum)
{
	static int	ix1 , ix2, ix3;
	static double	r [98];
	double		temp;
	static int	iff = 0;
	int		j;

	if (*idum < 0 || iff == 0)
	{
		iff = 1;
		ix1 = (IC1 - (*idum)) % M1;
		ix1 = (IA1 * ix1 + IC1) % M1;
		ix2 = ix1 % M2;
		ix1 = (IA1 * ix1 + IC1) % M1;
		ix3 = ix1 % M3;
		for (j = 1; j <= 97; j++)
		{
			ix1 = (IA1 * ix1 + IC1) % M1;
			ix2 = (IA2 * ix2 + IC2) % M2;
			r[j] = (ix1 + ix2 * RM2) * RM1;
		}
		*idum = 1;
	}
	ix1 = (IA1 * ix1 + IC1) % M1;
	ix2 = (IA2 * ix2 + IC2) % M2;
	ix3 = (IA3 * ix3 + IC3) % M3;
	j = (int)(1 + ((97 * ix3) / M3));
	if (j > 97 || j < 1)
		log_err("RAN1: This cannot happen.");
	temp = r[j];
	r[j] = (ix1 + ix2 * RM2) * RM1;
	return temp;
}

#undef M1
#undef IA1
#undef IC1
#undef RM1
#undef M2
#undef IA2
#undef IC2
#undef RM2
#undef M3
#undef IA3
#undef IC3

/*---------------------------------------------------------------------*/
#define E 2.71828182

double 
gs(double alpha)
{
	int		flag = 1;
	double		b      , p, x;

	b = (alpha + E) / E;
	do
	{
		p = b * ran1(&idum);
		if (p > 1.0)
		{
			x = -log((b - p) / alpha);
			if (pow(x, alpha - 1.0) >= ran1(&idum))
				flag = 0;
		} else
		{
			x = pow(p, 1.0 / alpha);
			if (x <= -log(ran1(&idum)))
				flag = 0;
		}
	}
	while (flag);
	return x;
}

 /*---------------------------------------------------------------------*/ 
double 
gcf(double alpha)
{
	int		flag = 1;
	static double	c1, c2, c3, c4, c5;
	static double	aprev = 0.0;
	double		tmp    , u1, u2, w = 0.0;

	if (alpha != aprev)
	{
		c1 = alpha - 1.0;
		tmp = 1.0 / c1;
		c2 = tmp * (alpha - 1.0 / (6.0 * alpha));
		c3 = 2.0 * tmp;
		c4 = c3 + 2.0;
		if (alpha > 2.5)
			c5 = 1.0 / sqrt(alpha);
		aprev = alpha;
	}
	do
	{
		u1 = ran1(&idum);
		u2 = ran1(&idum);
		if (alpha > 2.5)
		{
			u1 = u2 + c5 * (1.0 - 1.86 * u1);
			if (u1 <= 0.0 || u1 >= 1.0)
				continue;
		}
		w = c2 * u2 / u1;
		if ((c3 * u1 + w + 1.0 / w < c4) ||
		    (c3 * log(u1) - log(w) + w < 1.0))
			flag = 0;
	}
	while (flag);
	return c1 * w;
}


/*---------------------------------------------------------------------*/
double 
rgamma(double alpha)
{
	if (alpha == 1.0)
		return -log(ran1(&idum));
	if (alpha < 1.0)
		return gs(alpha);
	return gcf(alpha);
}

/*---------------------------------------------------------------------*/

double 
gamdev(int ia, int *idum)
{
	int		j;
	double		am     , e, s, v1, v2, x, y;

	if (ia < 1)
		log_err("Error in routine GAMDEV");
	if (ia < 6)
	{
		x = 1.0;
		for (j = 1; j <= ia; j++)
			x *= ran1(idum);
		x = -log(x);
	} else
	{
		do
		{
			do
			{
				do
				{
					v1 = 2.0 * ran1(idum) - 1.0;
					v2 = 2.0 * ran1(idum) - 1.0;
				} while (v1 * v1 + v2 * v2 > 1.0);
				y = v2 / v1;
				am = ia - 1;
				s = sqrt(2.0 * am + 1.0);
				x = s * y + am;
			} while (x <= 0.0);
			e = (1.0 + y * y) * exp(am * log(x / am) - s * y);
		} while (ran1(idum) > e);
	}
	return x;
}
/*---------------------------------------------------------------------*/

#define PI 3.141592654

double 
poidev(double xm, int *idum)
{
	static double	sq, alxm, g, oldm = (-1.0);
	double		em     , t, y;

	if (xm < 12.0)
	{
		if (xm != oldm)
		{
			oldm = xm;
			g = exp(-xm);
		}
		em = -1;
		t = 1.0;
		do
		{
			em += 1.0;
			t *= ran1(idum);
		} while (t > g);
	} else
	{
		if (xm != oldm)
		{
			oldm = xm;
			sq = sqrt(2.0 * xm);
			alxm = log(xm);
			g = xm * alxm - gammln(xm + 1.0);
		}
		do
		{
			do
			{
				y = tan(PI * ran1(idum));
				em = sq * y + xm;
			} while (em < 0.0);
			em = floor(em);
			t = 0.9 * (1.0 + y * y) * exp(em * alxm - gammln(em + 1.0) - g);
		} while (ran1(idum) > t);
	}
	return em;
}

/* #undef PI */
/*---------------------------------------------------------------------*/

void 
read_var(char *line, char *result)
{

	char		line1     [256];
	int		i;

	sprintf(line1, "%s", line);
	printf(line1);
	fgets(result, 250, stdin);
	result[strlen(result) - 1] = '\0';
	/* printf("\n You entered %s which is %d characters long\n",result,strlen(result)); */
	if (outfile != stdout)
	{
		for (i = 0; i < (int)minim(strlen(line1), 255); i++)
			if (line1[i] == (char)'\n')
				line1[i] = '\0';
		fprintf(outfile, "\"%s\",%s\n", line1, result);
	}
}

 /*---------------------------------------------------------------------*/ 

char           *
cvector(int nl, int nh)
{
	char           *v;

	v = (char *)malloc((unsigned)(nh - nl + 1) * sizeof(char));
	if (!v)
		log_err("allocation failure in cvector()");
	return v - nl;
}

unsigned char  *
ucvector(int nl, int nh)
{
	unsigned char  *v;

	v = (unsigned char *)malloc((unsigned)(nh - nl + 1) * sizeof(unsigned char));
	if (!v)
		log_err("allocation failure in cvector()");
	return v - nl;
}

int            *
ivector(int nl, int nh)
{
	int            *v;

	v = (int *)malloc((unsigned)(nh - nl + 1) * sizeof(int));
	if (!v)
		log_err("allocation failure in ivector()");
	return v - nl;
}

unsigned int   *
uvector(int nl, int nh)
{
	unsigned int   *v;

	v = (unsigned int *)malloc((unsigned)(nh - nl + 1) * sizeof(unsigned int));
	if (!v)
		log_err("allocation failure in uvector()");
	return v - nl;
}

unsigned long  *
ulvector(int nl, int nh)
{
	unsigned long  *v;

	v = (unsigned long *)malloc((unsigned)(nh - nl + 1) * sizeof(long));
	if (!v)
		log_err("allocation failure in ulvector()");
	return v - nl;
}

double         *
dvector(int nl, int nh)
{
	double         *v;

	v = (double *)malloc((unsigned)(nh - nl + 1) * sizeof(double));
	if (!v)
		log_err("allocation failure in dvector()");
	return v - nl;
}


int           **
imatrix(int nrl, int nrh, int ncl, int nch)
{
	int		i         , **m;

	m = (int **)malloc((unsigned)(nrh - nrl + 1) * sizeof(int *));
	if (!m)
		log_err("allocation failure 1 in imatrix()");
	m -= nrl;

	for (i = nrl; i <= nrh; i++)
	{
		m[i] = (int *)malloc((unsigned)(nch - ncl + 1) * sizeof(int));
		if (!m[i])
			log_err("allocation failure 2 in imatrix()");
		m[i] -= ncl;
	}
	return m;
}

unsigned int  **
umatrix(int nrl, int nrh, int ncl, int nch)
{
	int		i;
	unsigned int  **m;

	m = (unsigned int **)malloc((unsigned)(nrh - nrl + 1) * sizeof(unsigned int *));
	if (!m)
		log_err("allocation failure 1 in umatrix()");
	m -= nrl;

	for (i = nrl; i <= nrh; i++)
	{
		m[i] = (unsigned int *)malloc((unsigned)(nch - ncl + 1) * sizeof(unsigned int));
		if (!m[i])
			log_err("allocation failure 2 in umatrix()");
		m[i] -= ncl;
	}
	return m;
}

unsigned long **
ulmatrix(int nrl, int nrh, int ncl, int nch)
{
	int		i;
	unsigned long **m;

	m = (unsigned long **)malloc((unsigned)(nrh - nrl + 1) * sizeof(unsigned long *));
	if (!m)
		log_err("allocation failure 1 in ulmatrix()");
	m -= nrl;

	for (i = nrl; i <= nrh; i++)
	{
		m[i] = (unsigned long *)malloc((unsigned)(nch - ncl + 1) * sizeof(unsigned long));
		if (!m[i])
			log_err("allocation failure 2 in ulmatrix()");
		m[i] -= ncl;
	}
	return m;
}

void 
free_ulmatrix(unsigned long **m, int nrl, int nrh, int ncl, int nch)
{
	int		i;

	for (i = nrh; i >= nrl; i--)
		free((void *)(m[i] + ncl));
	free((void *)(m + nrl));
}


void 
free_imatrix(int **m, int nrl, int nrh, int ncl, int nch)
{
	int		i;

	for (i = nrh; i >= nrl; i--)
		free((void *)(m[i] + ncl));
	free((void *)(m + nrl));
}


double        **
dmatrix(int nrl, int nrh, int ncl, int nch)
{
	int		i;
	double        **m;

	m = (double **)malloc((unsigned)(nrh - nrl + 1) * sizeof(double *));
	if (!m)
		log_err("allocation failure 1 in dmatrix()");
	m -= nrl;

	for (i = nrl; i <= nrh; i++)
	{
		m[i] = (double *)malloc((unsigned)(nch - ncl + 1) * sizeof(double));
		if (!m[i])
			log_err("allocation failure 2 in dmatrix()");
		m[i] -= ncl;
	}
	return m;
}

void 
free_dmatrix(double **m, int nrl, int nrh, int ncl, int nch)
{
	int		i;

	for (i = nrh; i >= nrl; i--)
		free((void *)(m[i] + ncl));
	free((void *)(m + nrl));
}

char          **
cmatrix(int nrl, int nrh, int ncl, int nch)
{
	int		i;
	char          **m;

	m = (char **)malloc((unsigned)(nrh - nrl + 1) * sizeof(char *));
	if (!m)
		log_err("allocation failure 1 in cmatrix()");
	m -= nrl;

	for (i = nrl; i <= nrh; i++)
	{
		m[i] = (char *)malloc((unsigned)(nch - ncl + 1) * sizeof(char));
		if (!m[i])
			log_err("allocation failure 2 in cmatrix()");
		m[i] -= ncl;
	}
	return m;
}


unsigned char **
ucmatrix(int nrl, int nrh, int ncl, int nch)
{
	int		i;
	unsigned char **m;

	m = (unsigned char **)malloc((unsigned)(nrh - nrl + 1) * sizeof(unsigned char *));
	if (!m)
		log_err("allocation failure 1 in cmatrix()");
	m -= nrl;

	for (i = nrl; i <= nrh; i++)
	{
		m[i] = (unsigned char *)malloc((unsigned)(nch - ncl + 1) * sizeof(unsigned char));
		if (!m[i])
			log_err("allocation failure 2 in cmatrix()");
		m[i] -= ncl;
	}
	return m;
}



void 
free_cmatrix(char **m, int nrl, int nrh, int ncl, int nch)
{
	int		i;

	for (i = nrh; i >= nrl; i--)
		free((void *)(m[i] + ncl));
	free((void *)(m + nrl));
}


void 
free_ucmatrix(unsigned char **m, int nrl, int nrh, int ncl, int nch)
{
	int		i;

	for (i = nrh; i >= nrl; i--)
		free((void *)(m[i] + ncl));
	free((void *)(m + nrl));
}

void 
free_cvector(char *v, int nl, int nh)
{
	free((void *)(v + nl));
}

void 
free_ucvector(unsigned char *v, int nl, int nh)
{
	free((void *)(v + nl));
}


void 
free_ivector(int *v, int nl, int nh)
{
	free((void *)(v + nl));
}

void 
free_ulvector(unsigned long *v, int nl, int nh)
{
	free((void *)(v + nl));
}
void 
free_uvector(unsigned int *v, int nl, int nh)
{
	free((void *)(v + nl));
}

void 
free_dvector(double *v, int nl, int nh)
{
	free((void *)(v + nl));
}

/*---------------------------------------------------------------------*/

double 
gammln(double xx)
{
	double		x      , tmp, ser;
	static double	cof[6] = {76.18009173, -86.50532033, 24.01409822,
	-1.231739516, 0.120858003e-2, -0.536382e-5};
	int		j;

	x = xx - 1.0;
	tmp = x + 5.5;
	tmp -= (x + 0.5) * log(tmp);
	ser = 1.0;
	for (j = 0; j <= 5; j++)
	{
		x += 1.0;
		ser += cof[j] / x;
	}
	return -tmp + log(2.50662827465 * ser);
}

/*---------------------------------------------------------------------*/
double 
rbeta(double u, double v)
{
	double		r;

	r = rgamma(u);
	return (r / (r + rgamma(v)));
}

/*---------------------------------------------------------------------*/