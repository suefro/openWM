/*
Spoctení koeficientu A, B, C pro vypocet teploty z odporu na NTC termistoru pres Stein-Hart rovnici.
Porovnání se zjednodušeným beta-modelem.
Generování modelu predpripravených hodnot do mikrokontorléru.
dodelat:
-osetreni vstupu
*/
#include <iostream>
#include <cmath>
#include <conio.h>
#include <windows.h>
#include <fstream>

using namespace std;
//global variables
float R;
float R1;
float R2;
float R3;
float T1;
float T2;
float T3;

float KtoCelsius (float k)
{
    return k-273.15;
}
float TtoKelvin (float c)
{
    return c+273.15;
}

//výber koeficientu A, B nebo C
float koeficient (string TypKoeficient)
{
/*
vypocet koeficientù A,B,C
*/
float L1;
float L2;
float L3;
float Y1;
float Y2;
float Y3;
float A;
float B;
float C;

L1=log(R1); // ln()
L2=log(R2);
L3=log(R3);

Y1=1/TtoKelvin(T1);
Y2=1/TtoKelvin(T2);
Y3=1/TtoKelvin(T3);

float y2;
float y3;

y2=(Y2-Y1)/(L2-L1);
y3=(Y3-Y1)/(L3-L1);

C=((y3-y2)/(L3-L2))*(1/(L1+L2+L3));
B=y2-C*(pow(L1,2)+(L1*L2)+ pow(L2,2));
A=Y1-(B + pow(L1,2)*C)*L1;

if ("C" == TypKoeficient)return C;

if ("B" == TypKoeficient)return B;

if ("A" == TypKoeficient)return A;
}

float SHmodel (float R)
{
//Výpocet teploty S-H model
float T;
T=1/(koeficient("A")+koeficient("B")*log(R)+koeficient("C")*pow((log(R)),3));
return T;
}

float Bkoeficient ()
{
/*Výpocet Beta koeficientu pro Beta  model*/
return 1/((1/TtoKelvin(T1))-(1/TtoKelvin(T2)))*log(R1/R2);

}

float Bmodel (float R)
{
/*Výpocet teploty Beta-model*/

float T;
T=1/((1/TtoKelvin(T2))+(1/Bkoeficient())*log(R/R2));

return T;
}

void Uloz_koef()
{
 ofstream zapis;
 zapis.open("koeficienty.txt");

 //zapis << "R1:" << R1 << ";" << "R2:" << R2 << ";" << "R3:" << R3 << ";" << "T1:" << T1 << ";"<< "T2:" <<T2 << ";" << "T3:" << T3 << ";" << "A:" << koeficient("A") << ";" << "B:" << koeficient("B") << ";" << "C:" << koeficient("C") << ";" << "Beta:" << Bkoeficient() <<endl;
 zapis << "R1:" << R1 << endl;
 zapis << "R2:" << R2 << endl;
 zapis << "R3:" << R3 << endl;
 zapis << "T1:" << T1 << endl;
 zapis << "T2:" <<T2 << endl;
 zapis << "T3:" << T3 << endl;
 zapis << "A:" << koeficient("A") << endl;
 zapis << "B:" << koeficient("B") << endl;
 zapis << "C:" << koeficient("C") << endl;
 zapis << "Beta:" << Bkoeficient() <<endl;

 zapis.close();
}

void Generovani_SH()
{
    int rmin;
    int rmax;
   cout << "Zadejte minimalni odpor v Ohm:";
   cin >> rmin;
   cout << "Zadejte maximalni odpor v Ohm:";
   cin >> rmax;

   ofstream zapis;
   zapis.open("S-H.txt");

   for (int r = rmin; r < rmax; r++ )
   {
     zapis << r << ";" << KtoCelsius(SHmodel(r)) << endl;
   }
   zapis.close();

}

void Generovani_B()
{
    int rmin;
    int rmax;
   cout << "Zadejte minimalni odpor v Ohm:";
   cin >> rmin;
   cout << "Zadejte maximalni odpor v Ohm:";
   cin >> rmax;

   ofstream zapis;
   zapis.open("Beta.txt");

   for (int r = rmin; r < rmax; r++ )
   {
     zapis << r << ";" << KtoCelsius(Bmodel(r)) << endl;
   }
   zapis.close();

}

int main()
{
    cout << "Vypocet teploty pro NTC termistory (S-H model,B-model)" <<endl;
    cout << "------------------------------------------------------" <<endl;
    cout << "Zadejte Teplotu T1 v Celsiech:";
    cin >> T1;
    cout << "Zadejte Odpor R1 v Ohm:";
    cin >> R1;
    cout << "Zadejte Teplotu T2 v Celsiech:";
    cin >> T2;
    cout << "Zadejte odpor R2 v Ohm:";
    cin >> R2;
    cout << "Zadejte Teplotu T3 v Celsiech:";
    cin >> T3;
    cout << "Zadejte odpor R3 v Ohm:";
    cin >> R3;
    system("cls");
    cout << "Vypocet teploty pro NTC termistory (S-H model,B-model)\n------------------------------------------------------\n n -Novy vypocet\n s -Vypoctene koeficienty pro S-H model\n b -Vypocteny koeficient Beta pro B-model\n r -Vypocet konkretniho odporu \n c -Ulozeni koeficientu\n u -Vygenerovani rozsahu\n------------------------------------------------------\nk..konec\n######################################################\n";

    char volba;

do
 {


 volba=getch();
 system("cls");
    cout << "Vypocet teploty pro NTC termistory (S-H model,B-model)\n------------------------------------------------------\n n -Novy vypocet\n s -Vypoctene koeficienty pro S-H model\n b -Vypocteny koeficient Beta pro B-model\n r -Vypocet konkretniho odporu \n c -Ulozeni koeficientu\n u -Vygenerovani rozsahu\n------------------------------------------------------\nk..konec\n######################################################\n";
 switch (volba)
 {
 case 'n':
    cout << "Zadejte Teplotu T1 v Celsiech:";
    cin >> T1;
    cout << "Zadejte Odpor R1 v Ohm:";
    cin >> R1;
    cout << "Zadejte Teplotu T2 v Celsiech:";
    cin >> T2;
    cout << "Zadejte odpor R2 v Ohm:";
    cin >> R2;
    cout << "Zadejte Teplotu T3 v Celsiech:";
    cin >> T3;
    cout << "Zadejte odpor R3 v Ohm:";
    cin >> R3;
    system("cls");
 break;
 case 's':
    cout << "Teplota T1 v K:" << TtoKelvin(T1)<< " v celsiech:" << T1 << "; Odpor R1 v Ohm:" << R1 << endl;
    cout << "Teplota T2 v K:" << TtoKelvin(T2)<< " v celsiech:" << T2 << "; Odpor R1 v Ohm:" << R2 << endl;
    cout << "Teplota T3 v K:" << TtoKelvin(T3)<< " v celsiech:" << T3 << "; Odpor R1 v Ohm:" << R3 << endl;
    cout << "------------------------------------------------------" <<endl;
    cout << "Koeficient A: "<< koeficient("A") << endl;
    cout << "Koeficient B: "<< koeficient("B") << endl;
    cout << "Koeficient C: "<< koeficient("C") << endl;
 break;
 case 'b':
    cout << "Teplota T1 v K:" << TtoKelvin(T1)<< " v celsiech:" << T1 << "; Odpor R1 v Ohm:" << R1 << endl;
    cout << "Teplota T2 v K:" << TtoKelvin(T2)<< " v celsiech:" << T2 << "; Odpor R1 v Ohm:" << R2 << endl;
    cout << "Teplota T3 v K:" << TtoKelvin(T3)<< " v celsiech:" << T3 << "; Odpor R1 v Ohm:" << R3 << endl;
    cout << "------------------------------------------------------" <<endl;
    cout << "Koeficient Beta: "<< Bkoeficient() << endl;
 break;
 case 'r':
    cout << "Zadejte pozadovany odpor v Ohm:";
    cin >> R;
    cout << "Vypoctena teplota S-H model v celsiech:"<< KtoCelsius(SHmodel(R)) << endl;
    cout << "Vypoctená teplota B-model v celsiech:"<< KtoCelsius(Bmodel(R)) << endl;
    cout << "Rozdil obou modelu:"<< KtoCelsius(SHmodel(R))-KtoCelsius(Bmodel(R)) << endl;
 break;
 case 'c':
    Uloz_koef();
    cout << "Ulozeno!";
 break;
 case 'u':
    cout << "Stein-Hart model" <<endl;
    Generovani_SH();
    cout << "Beta-model" <<endl;
    Generovani_B();
    cout << "Vygenerovano do souboru!";
 break;

case 'k':
cout << "##############" <<endl;
cout << "Konec programu"<<endl;
cout << "##############" <<endl;

break;
 default:
cout << "##############" <<endl;
cout << "Neplatna volba"<<endl;
cout << "##############" <<endl;

 }

 }
 while (volba!='k');

return 0;
}
