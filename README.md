# MiShell – Rapport de Développement

## Vue d'ensemble

MiShell est un mini-shell implémenté en C, développé dans le cadre d'un projet scolaire. Ce projet m'a permis d'explorer les mécanismes de base d'un interpréteur de commandes : parsing, exécution, gestion des processus enfants, et opérateurs logiques.

## Démarche de développement

### Phase 1 : Commandes simples sans arguments

Au départ, j'ai commencé par l'essentiel : **exécuter des commandes simples**. L'objectif était de pouvoir saisir une commande (ex. `pwd`, `ls`) et l'afficher correctement.

- Création des commandes built-in : `cd`, `pwd`, `echo`, `exit`
- Structure de base sans fonctions au début
- Une boucle simple qui lit l'entrée et exécute

### Phase 2 : Exécution de commandes externes

Ensuite, j'ai réalisé que les utilisateurs voudraient exécuter **n'importe quelle commande**, pas juste mes built-in. J'ai donc ajouté la capacité à lancer des commandes externes via `execvp()` en créant un processus enfant (`fork()`).

**Décision clé** : Si une commande n'était pas un built-in, au lieu de lever une erreur, je **délègue son exécution au processus fils** qui va la rechercher dans le `PATH` du système. Cela s'est avéré **très utile par la suite** car cette approche m'a permis de gérer facilement les opérateurs complexes.

### Phase 3 : Arguments et redirections

J'ai ensuite ajouté la **gestion des arguments** (`ls -al`) et des **redirections** (`>`, `>>`, `<`). Le parsing est devenu plus complexe :

- Tokeniser la ligne de commande
- Détecter les redirections
- Appliquer les redirections dans le processus enfant avec `dup2()`

### Phase 4 : Les opérateurs – Ma plus grande difficulté

C'est ici que j'ai **vraiment eu du mal**. Je voulais supporter `&&` (AND logique), `||` (OR logique) et les **pipes** (`|`).

**Les défis** :

- Comment splitter correctement la ligne sur plusieurs opérateurs ?
- Comment gérer les pipes qui eux-mêmes changent la structure (redirection interprocess) ?
- Comment garder un code **lisible et maintenable** sans dupliquer la logique ?

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

### Répartition des responsabilités

| Fonctionnalité                     | Géré par MiShell     | Délégué à sh -c |
| ---------------------------------- | -------------------- | --------------- |
| Parsing &&                         | ✅                   | ❌              |
| Parsing \|\|                       | ❌                   | ✅              |
| Parsing pipes                      | ❌                   | ✅              |
| Exécution séquentielle avec &&     | ✅                   | ❌              |
| Exécution conditionnelle avec \|\| | ❌                   | ✅              |
| Gestion des pipes                  | ❌                   | ✅              |
| Redirections <, >, >>              | ✅                   | ❌              |
| Commandes built-in                 | ✅                   | ❌              |
| Commandes externes                 | ✅ (via fork/execvp) | ❌              |
| Détection background &             | ✅                   | ❌              |

## Points clés de la réflexion

1. **Ne pas réinventer la roue** : Les pipes sont complexes, le shell les gère bien → pourquoi pas le laisser faire ?
2. **Simplicité > Complexité** : Un code simple et lisible vaut mieux qu'un code "optimal" mais incompréhensible
3. **Itération progressive** : Commencer simple, puis ajouter des fonctionnalités une par une
4. **Délégation intelligente** : Utiliser les outils disponibles (`sh -c`, `fork()`, `execvp()`) plutôt que tout implémenter from scratch
5. **Refactoring indispensable** : Le code qui fonctionne n'est pas le code final – prendre le temps de restructurer, documenter et nettoyer est essentiel pour la maintenabilité

## Difficultés rencontrées et solutions

| Difficulté                 | Cause                                                    | Solution                                                       |
| -------------------------- | -------------------------------------------------------- | -------------------------------------------------------------- |
| Parsing complexe           | Tenter de gérer tous les opérateurs à la fois            | Split uniquement sur `&&`, déléguer les pipes au shell         |
| Pipes non fonctionnels     | Tentative de créer une structure complexe pour les pipes | Stocker en `pipeline_cmd` et passer à `sh -c`                  |
| Redirections sur builtins  | Pas de gestion du fork pour les builtins                 | Ajouter fork si redirection détectée                           |
| Mémoire invalide           | `strtok()` modifie la chaîne, puis on la free            | Ajouter `strdup()` sur les noms de fichiers                    |
| Code difficile à maintenir | Tout dans une seule fonction, pas de structure           | Refactoring complet : extraction de fonctions, noms explicites |

## Commandes supportées

- **Builtins** : `cd`, `pwd`, `echo`, `exit`
- **Externes** : Toute commande disponible dans le `PATH`
- **Opérateurs** :
  - `&&` (AND logique) – géré directement par MiShell
  - `||` (OR logique) – délégué au shell système via `sh -c`
  - `&` (background)
- **Pipes** : `|` (délégués au shell système via `sh -c`)
- **Redirections** : `<` (input), `>` (output), `>>` (append)

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

- Historique des commandes
- Alias et variables d'environnement

## Licence

Voir [LICENSE](LICENSE).
