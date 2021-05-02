Dumitrescu Andrei 323CC

Implementarea temei in C++:

Fisierul clienTCP.cpp -> flow:
    - verific daca parametrii dati din linia de comanda sunt buni
    - creez un socket
    - completez informatiile din adresa sv
    - ma conectez la server
    - trimit Id-ul clientului la server
    - foloset un FD_SET pentru multiplexare intre sv si STDIN
    - daca primesc informatie de la STDIN, citesc comanda data
        si verific daca este una valida.
        - daca nu este afisez ca acea comanda nu eset valida
    - daca este de la server, folosesc while pentru receive
        din cauza flow-ului are de la TCP, pentru a nu pierde
        vreun mesaj sau pentru a nu le combina.
        - daca este exit, incid clientul
        - daca este subscribe sau unsubscribe, afisez mesajul
        - in celalalte cazuri afisez mesajul

Flow-ul serverului:
    - verific daca parametrii dati din linia de comanda sunt buni
    - creez socketii pentru TCP si UDP
    - fac bind pe ambii socketi
    - ascult de pe socketul TCP
    - folosesc un FD_SET pentru pentru a vedea de p ce fd vine informatia
    - introduc in set STDIN, socketul de TCP si cel de UDP
    - selectez de fiecare data FDS de pe care vine informatia
    case UDP:
        - preiau informatia de la client si o parsez conform enuntului
        - o pun intr-o structura de tp udp_message
        - recreez mesajul initial si parcurg toti clientii abonati la
            acel topic. Daca clientul este abonat, atunci ii transmit
            mesajul, iar daca nu atunci il salvez intr-o coada ( in 
            cazul in care SF este 1)
    case TCP:
        - un nou clint.
        - accept conexiunea si preiau IDul de la acesta
        - verific daca este deja conectat, caz in care intrerup conexiunea
            si afisez un mesaj specific
        - verific daca a mai fost in trecut conectat, caz in care
            ii trimit mesajele restante
        - il retin intr-un hashmap
    case STDIN:
        - citesc de la tastatura comanda
        - daca este exit, inchid sv si clientii.
        - daca nu afisez faptul ca este comanda gresita.
    default:
        - informatie de la clienti
        - daca nr de bytes de la client este 0, atunci clientul
            se deconecteaza si aisez informatia aceasta
        - in orice alt acz, primesc un mesaj de la el
        - verific daca este subscribe sau unsubscribe
        - parsez informatia si il pun in hashmapul potrivit
        - trimit un mesaj de rapuns inapoi

Structuri folosite:
    - unordered_map pentru ca toate interogarile, insertiile si tergerile
        sa fie facute in O(1) -> rapiditate
    - vector din STL -> usor defolosit
    - string din STL -> usor de folosit
    - udp_message -> pentru a retine informatiile unui pachet udp