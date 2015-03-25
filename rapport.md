% Rapport du TP2
% Vincent Antaki et Guillaume Poirier-Morency

# Fonctionnement

Les requêtes sont représentés sur des bytes au lieu d'utiliser un encodage basé
sur une chaîne de caractères.

L'identifiant du client est un entier non-signé `unsigned int`.

Les demandes relatives ressources sont des entiers signés `int`:

 - une valeur positive représente une allocation de ressources
 - une valeur négative une libération de ressources
 - zéro est neutre

Le nombre de ressources (`n`) total est connu, alors il n'est pas nécéssaire de
le transmettre dans la requête.

À la fin de la requête, un `bool` est rajouté pour indiquer si il s'agit de la
dernière requête.

```
bloc:   | client id | ressource 1 | ressource 2 | ... | ressource n | dernière?
offset: 0           4             8             12    4n            4n + 1
```

Le serveur renvoit 1 seul octet signé contenant la valeur de retour
conformément à la spécification de l'énoncé.

En résumé,

 1. le serveur ouvre le socket et écoute les connexions entrantes avec `listen`
 2. le client ouvre le socket avec `open`
 3. le client appelle `connect` pour établir une connexion avec le serveur
 4. un serveur accepte la demande de connection du client avec `accept` et ouvre
    un descripteur de fichier pour communiquer avec le client
 5. le client envoit une requête au serveur contenant un identifiant et des
    demandes de ressources
 6. le serveur lit la requête du client et applique l'algorithme du Banquier
 7. le serveur écrit la réponse au client
 8. le client lit la réponse du client
 9. le serveur libère son descripteur de fichier
 10. attente et retour à l'étape 1 si le serveur a retourner n > 0

La fonction `accept` du serveur a dû être protégée par une exclusion mutuelle
afin d'éviter que deux fils d'exécution serveur acceptent la même requête
client.

Pour éviter de l'attente active au niveau des clients et serveurs, nous avons
utilisé ouvert le socket avec le mode bloquant. Toutes les opérations se
mettent en attente tant qu'ils ne reçoivent pas de réponse à l'autre extrémité.

## Question 1

Pour garantir que le système ne finira jamais en _deadlock_, nous avons pris
soin de :

 - ne pas créer de cycle de verrou au sein du programme
 - appliquer l'algorithme du Banquier et ne jamais transiter vers un état
   _unsafe_

## Question 2

Un des cas problématique au niveau de la corruption de données était d'avoir
plusieurs fils d'exécutions qui altéraient les compteurs statiques aux niveaux
des serveurs et des clients.

L'incrémentation est une opération qui comporte une lecture et une écriture
d'un espace mémoire. Or, deux threads peuvent lire la même valeur à un moment
donné, l'incrémenter et la réassigner dans l'espace mémoire. Le compteur aura
été incrémenté de `1` au lieu de `2`.

## Question 3

Pour synchroniser à la fin, le client indique dans sa requête si il s'agit de
la dernière requête. Quand tous les client on indiqué qu'ils ont terminé, les
fils d'exécutions serveurs peuvent se terminer proprement.

De plus, avec les prises bloquantes, un serveur ne peut pas se terminer tant
que le client qu'il sert n'a pas lu l'entier qu'il doit lui transmettre.
