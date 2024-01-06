Le projet est réparti sur 3 modules:
. un module consacré au parsing (parsing_jsh.c)
. le module principal consacré à la récupération et à l'exécution d'une commande (jsh.c)
. un module annexe contenant des fonctions auxiliaires utiles aux deux modules précédents (toolbox.c).

Deux structures de données sont utilisées pour le projet: une structure Job représentant un job et une structure Command représentant une commande.

Précisions sur la structure Command:

Après la lecture et la récupération d'une ligne de commande, celle-ci est envoyée au module de parsing.
Celui-ci renvoie ensuite une unique structure Command, représentant la dernière commande de la pipeline (éventuellement constituée d'une seule commande) correspondant à la ligne de commande.
La structure Command fait référence à elle-même: elle contient d'autres structures Command. Ainsi, la structure Command représentant la dernière commande de la pipeline tapée par l'utilisateur contient un champ "input", qui correspond à la commande qui la précède dans la pipeline. Cette dernière contient elle-même une commande dans son champ input correspondant à celle qui la précède, etc.
Toutes les commandes de la pipeline contiennent également un champ accueuillant les éventuelles substitutions qu'elles utilisent (commandes à exécuter pour constituer leurs arguments).
Le caractère récursif des structures de données Command induit le fait que certaines fonctions traitant les Command sont elles-mêmes récursives. C'est le cas par exemple des fonctions execute_command (prépare et initie l'exécution d'une commande) et free_command (libère la mémoire associée à une structure Command).

