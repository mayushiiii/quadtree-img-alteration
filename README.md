# Comprimarea / Decomprimarea / Oglindirea imaginilor folosind arbori cuaternari

Programul lucreaza cu imagini in format .ppm, iar plecand de la structura lor caracteristica se realizeaza o serie de modificari asupra lor. Pixelii sunt reprezentati in forma unor frunze de arbori cuaternari, iar imaginea in forma unui arbore de pixeli.
Am inclus si cateva teste in scopul de a demonstra ca programul realizeaza operatiunile cu succes. Operatiile posibile sunt urmatoarele si pot si apelate astfel:
- Compresia unei imagini: `-c [limita_compresie] [nume_fisier_necomprimat] [nume_fisier_final]`
- Decompresia unei imagini: `-d [nume_fisier_comprimat] [nume_fisier_final]`
- Oglindirea orizontala/verticala a unei imagini : `-m h/v [limita_compresie] [nume_fisier_necomprimat] [nume_fisier_final]`

Mai jos se intra in detaliu legat de aceste operatii, urmarind elementele cheie ale programului.

## Main
In main se apeleaza functiile care corespund cu cerintele date de argumente: comprimarea, decomprimarea, si oglindirea.
Toate aceste 3 mari cerinte au in comun diverse instructiuni, de exemplu:
- decomprimarea e opusul comprimarii cu mici exceptii
- oglindirea pleaca tot de la comprimare

Oglindirea si comprimarea incep la fel, in sensul ca intai trebuie extrasa matricea de pixeli a fisierului, apoi realizat arborele de comprimare.  Diferenta vine in prelucrarile facute dupa acest punct: la compresie se realizeaza vectorul de comprimare, iar apoi introducerea acestuia in fisierul  tip .out, in timp ce la oglindire se stabileste tipul de oglindire, se aplica, iar apoi se scrie un nou fisier .ppm (care poate fi si vizualizat)

Decompresia foloseste acelasi tipar de fisier ca si cele rezultate in urma compresiei, deci are loc citirea formatului respectiv, apoi vin pasii opusi algoritmului de compresie: se realizeaza arborele de compresie pe baza vectorului citit, si se reconstituie imaginea finala prin algoritmul de decompresie.

## Image compression

imaginea finala depinde de limita aleasa pentru media culorilor pixelilor.

in functie se afla/extrag intai toate datele ce corespund nodului curent:
- aria pe care o acopera
- matricea de valori rgb care ii corespunde
- media valorilor rgb din matrice
- indexul la momentul respectiv

Se calculeaza mean-ul pe baza unei formule, iar apoi, comparand mean-ul cu limita noastra, decidem daca continuam sa apelam functia sau nu.

In main, cum am zis si mai sus, se afla nr de frunze iar apoi, avand toate datele
necesare, se introduc in fisier.

## Image decompression 

E in 90% din privinte inversul compresiei, singura treaba a fost ca nu am putut utiliza/afla matricea de valori rgb corespondenta fiecarul element din nod aici, deci am initializat-o cu null. Din cauza asta si in functia de free a arborelui are loc o verificare daca exista ceva de eliberat sau nu.

Extragem din fisierul .out intai numarul de noduri si de frunze, iar apoi tot vectorul corespondent imaginii. este apelata o functie care transforma vectorul in arbore; aici se atribuie valorile nodului si se continua pana nu mai sunt frunze neparcurse.

In main imi atribui caracteristicile imaginii, iar apoi apelez functia de decompresie, care introduce in mod recursiv (ca si cam orice din tema asta)  valorile rgb din noduri in matricea imaginii. odata ce matricea este determinata, se scriu datele in fisierul final.

## Image mirroring

Cum spuneam, inceputul este acelasi ca si cel de la algoritmul de compresie, avand in vedere ca trebuie citite aceleasi date din acelasi tip de fisier. Se extrage imaginea, se realizeaza arborele de compresie, se afla tipul de oglindire si apoi se realizeaza oglindirea in sine.

Aceasta consta pur si simplu in inversarea anumitor noduri din arbore, in functie de tipul de oglindire; pentru asta se apeleaza functia de swap.

Apelez functia de decompresie pentru a-mi reconstitui imaginea finala, si in final introduc datele (caracteristicile imaginii + matricea de pixeli) in fisierul final ppm.
_______________________________________________________________________________

In rest singura mentiune de facut ar mai fi catre functia de get_image, care citeste fisierul de input cu datele necesare, le introduce in structura cu datele imaginii, si apoi realizeaza matricea de pixeli pe baza valorilor rgb.
