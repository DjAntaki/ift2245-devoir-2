% Rapport du TP2
% Vincent Antaki et Guillaume Poirier-Morency

###Rapport tp2 2



#Critique face au tp :

Nous voulons dénoncer l'encodage horrible avec lequel on nous a demander de transmettre les données. On n'a que des int à transmettre et pourtant on nous demande de transmettre l'encodage est en char.

Calculer le taux de sureprésentation de l'encodage.

Les requêtes sont représentés sur des bytes au lieu d'utiliser un encodage basé
sur une chaine de caractères.

L'identifiant du client est un entier non-signé `unsigned int`.

Les demandes relatives ressources sont des entiers signés `int`:

 - une valeur positive représente une libération de ressources
 - une valeur négative une allocation.
 - zéro est neutre

Le nombre de ressources (`n`) total est connu, alors il n'est pas nécéssaire de 
le transmettre dans la requête.

```
bloc:   | client id | ressource 1 | ressource 2 | ... | ressource n |
offset: 0           4             8             12    4n            4n + 1
```

Le serveur renvoit 1 seul byte signé contenant la valeur de retour conformément 
à la spécification de l'énoncé.

# Fonctionnement général

 1. le client acquière la mutex pour utiliser le socket
 2. le client appelle `connect`
 3. un serveur accepte la demande de connection du client
 4. le client envoit une requête au serveur
 5. le serveur lit la requête du client
 6. le serveur écrit la réponse au client
 7. le client lit la réponse du client
 8. le client libère la mutex pour utiliser le socket
 9. attente et retour à l'étape 1 si le serveur a retourner n > 0

## Fonctionnement du client

Un seul fil d'exécution fait appel à `connect` à l'aide d'une exclusion 
mutuelle.

## Fonctionnement du serveur

Les fils d'exécution clients font un appel à `connect` 
Le client envoit une requête

Le _dispatch_ des fils d'exécution serveurs aux clients est simple: tous les 
serveurs se mettent en attente pour accepter une requête à l'aide d'une mutex.

Un seul socket s'occupe de la communication entre les fils d'exécutions clients et serveurs.
En terme d'opéraito
Les clients utilisent `connect` et le

