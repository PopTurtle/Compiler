# Compilateur réalisé en C (avec flex et bison)

## Infos

Le compilateur compile du code Algo (langage de programmtion Latex) en assembleur Asipro (https://github.com/NicolasBedon/asipro), un langage assembleur créé pour le cours de compilation.
Afin de compiler le compilateur les outils flex, bison et gcc sont nécessaire

# Fonctionnalités

## Code Algo

Le langage Algo s'écrit comme suit

```
\begin{algo}{puissance}{a, b, acc}
    \IF{b == 0}
        \RETURN{acc}
    \FI
    \RETURN{\CALL{puissance}{a, b - 1, acc * a}}
\end{algo}

\CALL{puissance}{3, 4, 1}
```

C'est assez explicite, voilà le jeu d'instructions implémenté par le compilateur :

```
\SET{var}{expression}
\IF code \ELSE code \FI
\DOWHILE{condition} code \OD
\DOFORI{var}{expression_debut}{expression_fin} \OD
\CALL{fonction}{parametres}
```

Les opérateurs des expressions :

```
+ - * /
== > >= < <=
&& || !
```

## Utilisation

Afin de compiler le compilateur, se rendre dans le dossier compiler, puis faire un `make`.

Pour compiler un fichier :

```
./algosipro < nom_fichier.algo > compiled.asipro
```

Pour afficher l'aide et les options :

```
./algosipro -h
```

Pour executer du code asipro : (voir sur le repos asipro)

```
asipro compiled.asipro compiled.sipro
sipro compiled.sipro
```

## Ajouts complémentaires

En plus de compiler le code le compilateur contient d'autres fonctionnalités, telles que:

- Vérification du code :
  - Vérifie que tous les chemins renvoient bien des valeurs
  - Vérifie que toutes les variables utilisées ont bien une valeur qui a été assignée
- Optimisation du code :
  - Suppresion de codes morts (Code après return, codes vides, condition non remplissables...)
  - Dérécursification de fonctions récursives terminales
  - Précalcule des expressions simples (8 + 4 \* 7 devient 36)
