# 🔥 Buffer Overflow Lab — Démonstration pédagogique (C / x86)

Ce dépôt présente **une vulnérabilité de type Buffer Overflow** sur un petit programme C volontairement vulnérable.  
L’objectif est **d’expliquer clairement** la vulnérabilité, **montrer ma démarche d’analyse**, et **mettre en avant mes compétences en sécurité applicative / exploitation**.

---

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

### 4. Conclusion
L’exploitation d’un buffer overflow n’est pas seulement un exercice offensif.  
Elle permet de comprendre **pourquoi les protections modernes existent** (ASLR, NX, Stack Canary, RELRO…).

### 5. Compilation
Sans les protections de sécurité (mode démonstration)
gcc vulnerable.c -o vuln -fno-stack-protector -z execstack


Vous pouvez activer l’ASLR systématiquement pour vos tests :

echo 0 | sudo tee /proc/sys/kernel/randomize_va_space

---