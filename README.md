# MiShell – Rapport de Développement

## Vue d'ensemble

MiShell est un mini-shell implémenté en C, développé dans le cadre d'un projet scolaire. Ce projet m'a permis d'explorer les mécanismes de base d'un interpréteur de commandes : parsing, exécution, gestion des processus enfants, et opérateurs logiques.

## Important : Clarification sur les fonctionnalités

Les redirections (`<`, `>`, `>>`), les opérateurs logiques (`&&`, `||`) et les pipes (`|`) **étaient obligatoires** selon le sujet du projet (FM02). Cependant, de ce que j'ai compris, le sujet ne demandait **pas** explicitement de les implémenter comme des **built-in** (fonctionnalités codées directement dans le shell).

Cela signifie que j'avais le choix entre :

1. **Les implémenter moi-même** (redirection via `dup2()`, parsing des opérateurs, etc.) – ce que j'ai fait au début puis que j'ai abandonné car trop complexe
2. **Les déléguer au shell système** via `sh -c` – ce que j'ai donc fait par la suite pour les pipes et `||`

Les seules commandes qui **devaient être obligatoirement built-in** étaient : `cd`, `pwd`, `echo`, `exit` (FM03).

## Démarche de développement

### Phase 1 : Commandes simples sans arguments

Au départ, j'ai commencé par l'essentiel : **exécuter des commandes simples**. L'objectif était de pouvoir saisir une commande (ex. `pwd`, `ls`) et l'afficher correctement.

- Création des commandes built-in : `cd`, `pwd`, `echo`, `exit`
- Structure de base sans fonctions au début
- Une boucle simple qui lit l'entrée et exécute

### Phase 2 : Exécution de commandes externes

Ensuite, pour exécuter **n'importe quelle commande**, pas juste mes built-in j'ai ajouté la capacité à lancer des commandes externes via `execvp()` en créant un processus enfant (`fork()`).

**Décision clé** : Si une commande n'était pas un built-in, au lieu de lever une erreur, je **délègue son exécution au processus fils** qui va la rechercher dans le `PATH` du système. Cela s'est avéré **très utile par la suite** car cette approche m'a permis de gérer facilement les opérateurs complexes.

### Phase 3 : Arguments et redirections

J'ai ensuite ajouté la **gestion des arguments** (`ls -al`) et des **redirections** (`>`, `>>`, `<`) – deux fonctionnalités obligatoires du sujet. Le parsing est devenu plus complexe :

- Tokeniser la ligne de commande
- Détecter les redirections
- Appliquer les redirections dans le processus enfant avec `dup2()`

### Phase 4 : Les opérateurs – Ma plus grande difficulté

Le sujet exigeait de supporter les opérateurs `&&` (AND logique), `||` (OR logique) et les **pipes** (`|`). C'est ici que j'ai **vraiment eu du mal** car au départ je pensais à tord que je devais les gérés totalement "built-in".

**Mes tentatives initiales** étaient trop complexes :

- Tentative 1 : Parser tous les opérateurs en même temps → code très imbriqué et difficile à suivre
- Tentative 2 : Stocker les commandes dans des structures complexes → trop de cas à gérer

**La solution finale** : J'ai opté pour une **approche minimaliste** :

1. **Split uniquement sur `&&`** → cela me donne au maximum 3 commandes (conformément à `MAX_CMDS`)
2. **Conserver les pipes ET les `||` à l'intérieur des commandes** → plutôt que de les parser moi-même, je les passe au shell système
3. **Créer une fonction `parse_single_command()`** qui traite **une seule commande** à la fois

Ainsi :

- `ps && ls` → deux commandes exécutées séquentiellement (géré par MiShell)
- `ls | grep ".txt"` → **une seule commande** passée à `sh -c` qui gère le pipe
- `false || echo "ok"` → **une seule commande** passée à `sh -c` qui gère le `||`
- `ps && ls | grep ".txt"` → deux commandes : `ps` d'un côté, `ls | grep ".txt"` de l'autre

**Avantage** : Le code devient **très lisible**. Les pipes et `||` ne sont pas mon problème – je laisse le shell s'en charger via `sh -c`. J'ai juste à gérer le split logique sur `&&`.

### Phase 5 : Redirections avec builtins

Un dernier défi : les builtins (`echo`, `pwd`) ne supportaient pas les redirections. J'ai ajouté une vérification : si un builtin a une redirection, il est exécuté via le processus enfant (comme une commande externe).

### Phase 6 : Historique des commandes – Une persistance bien pensée

Au début, j'avais une idée simple : sauvegarder chaque commande dans un fichier `mishell_history.txt`. Mais rapidement, je me suis heurté à un problème courant avec les chemins relatifs.

**Le problème** :

Quand j'utilisais un chemin relatif (`mishell_history.txt`), le fichier se créait dans le répertoire courant au démarrage du shell. Mais dès qu'un utilisateur faisait `cd`, le répertoire courant changeait ! La prochaine commande sauvegardée créait un **nouveau fichier** dans le nouveau répertoire au lieu de continuer d'écrire dans l'original.

**Mes tentatives** :

1. **Première idée** : Utiliser la variable `$HOME` pour stocker l'historique dans le home de l'utilisateur. Mais sur Windows/WSL, cette variable n'est pas toujours définie, et ce n'était pas vraiment ce que je voulais.

2. **Deuxième idée** : Garder un chemin relatif mais ajouter une variable globale `history_path` et la passer partout. Ça fonctionnait, mais c'était redondant et peu élégant.

**La solution finale** :

J'ai implémenté une **variable globale statique `initial_cwd`** qui sauvegarde le répertoire courant au démarrage du shell via `getcwd()`. Ensuite, peu importe où on navigue avec `cd`, l'historique est toujours écrit au même endroit : `<répertoire_de_lancement>/mishell_history.txt`.

```c
// Au démarrage
init_history_directory();  // Capture le cwd dans initial_cwd

// Pour chaque commande
save_in_history(line);     // Utilise toujours initial_cwd
```

## Architecture finale

```
parse_command() [MiShell]
  └─ Split sur && uniquement
  └─ parse_single_command() (pour chaque segment) [MiShell]
     └─ Détecte pipes ou || → stocke tout en pipeline_cmd [Délégation]
     └─ Sinon, tokenise et détecte redirections [MiShell]

execute_command() [MiShell]
  └─ execute_single_command() (pour chaque commande) [MiShell]
     └─ Si pipeline_cmd → run_pipeline_command() [sh -c]
     └─ Si builtin sans redirection → exécute directement [MiShell]
     └─ Sinon → run_external_command() [fork + execvp]
```

### Note sur les alias (FM07)

Pour les **alias du shell système** (comme `ll` pour `ls -al`), aucune implémentation spéciale n'est nécessaire dans MiShell. Puisqu'un alias n'est pas une commande built-in reconnue, il est automatiquement traité comme une **commande externe** et exécuté via `fork() + execvp()`. Le shell système se charge de résoudre l'alias. Cela fonctionne naturellement sans code supplémentaire.

### Répartition des responsabilités

| Fonctionnalité                     | Géré par MiShell     | Délégué à sh -c |
| ---------------------------------- | -------------------- | --------------- |
| Parsing &&                         | ✅                   | ❌              |
| Parsing \|\|                       | ❌                   | ✅              |
| Parsing pipes                      | ❌                   | ✅              |
| Exécution séquentielle avec &&     | ✅                   | ❌              |
| Exécution conditionnelle avec \|\| | ❌                   | ✅              |
| Gestion des pipes                  | ❌                   | ✅              |
| Redirection <, >, >>               | ✅                   | ❌              |
| Commandes built-in                 | ✅                   | ❌              |
| Commandes externes                 | ✅ (via fork/execvp) | ❌              |
| Détection background &             | ✅                   | ❌              |
| Historique persistant              | ✅                   | ❌              |
| Alias du shell système             | ❌                   | ✅              |

## Difficultés rencontrées et solutions

| Difficulté                        | Cause                                                    | Solution                                                       |
| --------------------------------- | -------------------------------------------------------- | -------------------------------------------------------------- |
| Parsing complexe                  | Tenter de gérer tous les opérateurs à la fois            | Split uniquement sur `&&`, déléguer les pipes au shell         |
| Pipes non fonctionnels            | Tentative de créer une structure complexe pour les pipes | Stocker en `pipeline_cmd` et passer à `sh -c`                  |
| Redirections sur builtins         | Pas de gestion du fork pour les builtins                 | Ajouter fork si redirection détectée                           |
| Mémoire invalide                  | `strtok()` modifie la chaîne, puis on la free            | Ajouter `strdup()` sur les noms de fichiers                    |
| Code difficile à maintenir        | Tout dans une seule fonction, pas de structure           | Refactoring complet : extraction de fonctions, noms explicites |
| Historique créé à mauvais endroit | Chemin relatif + `cd` = fichiers dupliqués               | Capturer le cwd au démarrage dans une variable globale         |
| Double free en mémoire            | Libération de `ParsedCommand` à deux endroits            | Créer `free_command()` et l'appeler une seule fois             |
| Warning snprintf troncature       | `snprintf()` pouvait tronquer le chemin d'historique     | Vérifier le retour de `snprintf()` au lieu de pré-calculer     |

## Commandes supportées

- **Builtins** : `cd`, `pwd`, `echo`, `exit`
- **Externes** : Toute commande disponible dans le `PATH`
- **Alias** : Les alias du shell système sont supportés (ex: `ll` s'il existe)
- **Opérateurs** :
  - `&&` (AND logique) – géré directement par MiShell
  - `||` (OR logique) – délégué au shell système via `sh -c`
  - `&` (background)
- **Pipes** : `|` (délégués au shell système via `sh -c`)
- **Redirections** : `<` (input), `>` (output), `>>` (append)
- **Historique** : Toutes les commandes sont enregistrées automatiquement dans `mishell_history.txt`

## Exemple d'utilisation

```sh
# Redirection + AND (géré par MiShell)
MiShell> $ echo Hello > test.txt && cat test.txt
Hello

# Pipe (délégué à sh -c)
MiShell> $ ls -al | grep ".c"
-rw-r--r-- ... functions.c
-rw-r--r-- ... mishell.c

# OR logique (délégué à sh -c)
MiShell> $ false || echo "Fallback executed"
Fallback executed

# AND géré par MiShell
MiShell> $ cd /tmp && pwd
/tmp

# Combinaison : AND (MiShell) + Pipe (sh -c)
MiShell> $ echo "test" && ls | head -n 2
test
file1
file2

MiShell> $ exit
Exiting MiShell...
```

## Construction

```sh
make all       # Compile tout
make clean     # Nettoie les objets
make doc       # Génère la doc Doxygen
```

## Dépendances

- Compilateur C (gcc)
- Bibliothèques C standard (stdio, stdlib, etc.)
- Fonctions POSIX (fork, wait, execvp) – **requiert Unix/Linux/WSL**

## TODO

- Variables d'environnement

## Licence

Voir [LICENSE](LICENSE).
