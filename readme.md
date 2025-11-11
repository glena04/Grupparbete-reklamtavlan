Reklamskyltsystem - C++ Lösning
Översikt
Detta program implementerar ett digitalt reklamskyltsystem där olika kunder får visning baserat på hur mycket de betalat. 
Systemet säkerställer att samma kund aldrig visas två gånger på raken.


<img width="734" height="499" alt="image" src="https://github.com/user-attachments/assets/35d2d356-40d1-4532-adce-24b52579cff3" />


Funktioner
Huvudfunktioner

Viktad slumpning: Kunder väljs med sannolikhet proportionell mot deras betalning.

Aldrig samma kund två gånger på raken: Systemet kommer ihåg senaste kunden.

Olika visningstyper: Text, scrollande text, blinkande text

Tidsbaserad logik: Svarte Petter visar olika meddelanden på jämna/ojämna minuter

20 sekunders visning: Varje reklam visas i exakt 20 sekunder



Kunder och deras betalningar

Kund	                      Betalning	Sannolikhet	Antal meddelanden

Hederlige Harrys Bilar	    5000 kr	  ~34.5%	       3

Farmor Ankas Pajer AB	     3000 kr  	~20.7%	       2

Svarte Petters Svartbyggen	1500 kr	  ~10.3%	       2   tidsbaserat)

Långbens detektivbyrå	     4000 kr	  ~27.6%	       2

IOT:s Reklambyrå	          1000 kr	  ~6.9%        	1


Syftet med denna grupp arbete är att:
 -Att arbeta i grupp via GitHub och vscode 
- Att använda C++ för att lösa ett praktiskt problem
- Att implementera slumpmässig visning med viktning
- Att tänka i objektorienterade termer (klasser, objekt, logik)
