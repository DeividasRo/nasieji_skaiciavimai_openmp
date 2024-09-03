#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <cmath>
#include <omp.h>

using namespace std;
//===== Globalus kintamieji ===================================================

int num_points = 5000;			 // Tasku skaicius (max 50000). Didinant ilgeja matricos skaiciavimo ir sprendinio paieskos laikas
int num_variables = 3;			 // Tasku, kuriuos reikia rasti, skaicius
int num_possible_locations = 60; // Galimu vietu naujiems objektams skaicius (pirmi taskai is masyvo)

double **points;		  // Masyvas taskams saugoti
double **distance_matrix; // Masyvas atstumu matricai saugoti

//===== Funkciju antrastes ====================================================

double get_time();										   // Funkcija laiko matavimui
void load_data();										   // Duomenu ikelimo is failo funkcija
double Haversine_distance(double, double, double, double); // Atstumo skaiciavimo pagal Haversino formule funkcija
double distance_from_matrix(int, int);					   // Atstumo paemimo is atstumu matricos funkcija
bool next_solution(int *);								   // Sekancio sprendinio generavimo funkcija
double evaluate_solution(int *);						   // Funkcija sprendinio tikslo funkcijos reiksmei ivertinti
unsigned int factorial(int);

//=============================================================================

int main(int argc, char **argv)
{
	srand(time(NULL));

	const int THREAD_NUM = argc > 1 ? atoi(argv[1]) : 1;
	omp_set_num_threads(THREAD_NUM);

	double t_0 = get_time(); // Programos vykdymo pradzios laiko fiksavimas

	load_data(); // Duomenu ikelimas is failo

	double t_1 = get_time(); // Duomenu ikelimo pabaigos laiko fiksavimas

	//-------------------------------------------------------------------------
	// Skaiciuojam atstumu matrica
	// Matrica yra "trikampe", nes atstumai nuo A iki B ir nuo B iki A yra lygus
	//-------------------------------------------------------------------------

	distance_matrix = new double *[num_points];

#pragma omp parallel for schedule(dynamic)
	for (int i = 0; i < num_points; i++)
	{
		distance_matrix[i] = new double[i + 1];
		for (int j = 0; j <= i; j++)
		{
			distance_matrix[i][j] = Haversine_distance(points[i][0], points[i][1], points[j][0], points[j][1]);
		}
	}

	double t_2 = get_time(); // Matricos skaiciavimo pabaigos laiko fiksavimas

	//-------------------------------------------------------------------------
	// Geriausio sprendinio pasieska pilno perrinkimo algoritmu
	// (angl. Complete Enumeration)
	//-------------------------------------------------------------------------
	int *best_solution = new int[num_variables];
	double f_best_solution = 1e10;

#pragma omp parallel
	{
		int *solution = new int[num_variables];
		double f_solution = 1e10;
		for (int i = 0; i < num_variables; i++)
		{
			solution[i] = i;
		}
		int combination_amount = (num_possible_locations * (num_possible_locations - 1) * (num_possible_locations - 2)) / factorial(num_variables);
		int start = omp_get_thread_num() * combination_amount / THREAD_NUM;
		int end = (omp_get_thread_num() + 1) * combination_amount / THREAD_NUM - 1;
		int k = 0;

		while (next_solution(solution) == true)
		{
			if (k >= start)
			{
				if (k == end)
				{
					break;
				}
				// cout << solution[0] << " " << solution[1] << " " << solution[2] << endl;
				f_solution = evaluate_solution(solution); // Sprendinio tikslo funkcijos skaiciavimas
#pragma omp critical
				{
					if (f_solution < f_best_solution)
					{								  // Tikrinam ar sugeneruotas sprendinys yra geresnis (mazesnis) uz geriausia zinoma
						f_best_solution = f_solution; // Jei taip, atnaujinam informacija apie geriausia zinoma sprendini
						for (int j = 0; j < num_variables; j++)
						{
							best_solution[j] = solution[j];
						}
					}
				}
			}
			k++;
		}
	}

	double t_3 = get_time();

	cout << "Geriausias rastas sprendinys (tasku indeksai duomenu masyve): ";
	for (int i = 0; i < num_variables; i++)
		cout << best_solution[i] << " ";
	cout << endl;
	cout << "Duomenu ikelimo laikas: " << t_1 - t_0 << " s." << endl;
	cout << "Atstumu matricos skaiciavimo laikas: " << t_2 - t_1 << " s." << endl;
	cout << "Sprendinio paieskos laikas: " << t_3 - t_2 << " s." << endl;
}

//=============================================================================
// Papildomos funkcijos. SIU FUNKCIJU LYGIAGRETINTI NEREIKIA.
//=============================================================================

unsigned int factorial(int n)
{
	if (n == 0)
		return 1;
	return n * factorial(n - 1);
}

double get_time()
{
	struct timeval laikas;
	gettimeofday(&laikas, NULL);
	double rez = (double)laikas.tv_sec + (double)laikas.tv_usec / 1000000;
	return rez;
}

//=============================================================================

void load_data()
{

	//----- Load demand points ------------------------------------------------
	FILE *f;
	f = fopen("lab_data.dat", "r");
	points = new double *[num_points];
	for (int i = 0; i < num_points; i++)
	{
		points[i] = new double[2];
		fscanf(f, "%lf%lf", &points[i][0], &points[i][1]);
	}
	fclose(f);
}

//=============================================================================

double Haversine_distance(double lat1, double lon1, double lat2, double lon2)
{
	double dlat = fabs(lat1 - lat2);
	double dlon = fabs(lon1 - lon2);
	double aa = pow((sin((double)dlat / (double)2 * 0.01745)), 2) + cos(lat1 * 0.01745) *
																		cos(lat2 * 0.01745) * pow((sin((double)dlon / (double)2 * 0.01745)), 2);
	double c = 2 * atan2(sqrt(aa), sqrt(1 - aa));
	double d = 6371 * c;
	return d;
}

//=============================================================================

double distance_from_matrix(int i, int j)
{
	if (i >= j)
		return distance_matrix[i][j];
	else
		return distance_matrix[j][i];
}

//=============================================================================

bool next_solution(int *solution)
{
	int k = num_variables;
	int n = num_possible_locations;

	int i = k - 1;
	while (i >= 0 && solution[i] == n - k + i)
		i--;
	if (i < 0)
		return false;
	solution[i]++;
	for (int j = i + 1; j < k; j++)
	{
		solution[j] = solution[j - 1] + 1;
	}
	return true;
}

//=============================================================================

double evaluate_solution(int *solution)
{
	double distance, min_distance, total_distance = 0;
	for (int i = 0; i < num_points; i++)
	{
		min_distance = 1e10;
		for (int j = 0; j < num_variables; j++)
		{
			distance = distance_from_matrix(i, solution[j]);
			if (distance < min_distance)
			{
				min_distance = distance;
			}
		}
		total_distance += min_distance;
	}
	return total_distance;
}

//=============================================================================