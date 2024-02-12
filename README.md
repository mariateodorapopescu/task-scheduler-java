# tema2

Popescu Maria-Teodora
332CC

Asa cum s-a mention si in cerinta, implementarea temei s-a realizat in clasele MyDispatcher si MyHost.

1. MyDispatcher:

->addTask:

	am luat fiecare politica in parte si am încercat sa dau mai departe taskul acelui host care se potrivește conform regulii. 
	Astfel, m-am gândit ca: sa fac un switch (nu am vrut sa am prea multe if-uri, mi s-a părut redundant) si am luat pe tipuri. 
	Astfel, la round robin am pus sa incrementeze un număr care va reprezenta indicele hostului la care se va trimite, si l-am repetat la 0 când se ajungea la numărul de hosturi, ca sa nu depășească numărul de hosturi si cumva sa se cicleze. Am folosit variabila atomică ca sa nu aibă loc race condition. 
	La shortest queue am pus cozile pe o colectie ca sa iau minimul, adică pe cea cu lungimea cea mai mica, conform comparatorului. 
	La SITA a fost destul de simplu, am luat in funcție de enum unde sa se ducă, de data asta cu if. 
	La Least work left am procedat asemanator ca la shortest queue. 

->Aici a fost de implementat numai metoda addTask.


------------------------------------------------------------------------------------------------

2. MyHost:

-> In schimb, in clasa myHost am implementat mai multe metode, cum ar fi run, addTask, getQueueSize, getWorkLeft si shutdown, dar si coada din host. 

-> In mare, se bazează pe sincronizare, folosind un lock reentrant care sa semnaleze secțiunile critice si adăugarea sau scoaterea taskului din coada. Când se scoate din coada un task, se rulează. Taskul current, cel care rulează nu este cel din vârful cozii (aceasta se întâmplă numai la început). 

-- Queue:
	Astfel, am folosit o PriorityBlockingQueue ce asigură că task-urile sunt ordonate în funcție de prioritate și, în caz de egalitate, în funcție de timpul lor de start. 

-- run:
	In metoda run am simulat rularea unui task, făcând un busywaiting cu sleep, in funcție de durata. Știu ca in viața reală nu se întâmplă așa, adică nu rulează per total, ci rulează puțin si se oprește, dar măcar se poate aplica pentru anumite condiții, cum ar fi daca nu e preemptibil. 

-- addTask:
	In schimb, logica de a preempta (am aici o tentativa de a face asta) cu re-ordonarea in funcție de preemptare si priorități am realizat-o in addTask.

-- Shutdown:
	Shutdown se bazează pe schimbarea variabilei ok, care indică daca hostul rulează sau nu.

-- getQueueSize:
	am întors lungimea cozii si atât.

-- getWorkLeft:
Am făcut suma din timpii left ale tuturor taskurilor din coada hostului
