// - s'il n'y a pas de rectangle libre :
// -- agrandit l'image :
// --- ajoute le rectangle à la liste de rectangle libre
// - sinon :
// -- si le rectangle suffit :
// --- enlève le rectangle de la liste de rectangle libre
// -- sinon :
// --- coupe le rectangle et ajoute la chute à la liste de rectangle libre

struct Rectangle {
    int pos_x;
    int pos_y;
    int largeur;
    int hauteur;
};

std::vector<Rectangle> rectangles_libre;

enum {
    HAUTEUR_LARGEUR,
    HAUTEUR,
    LARGEUR,
    PLUS_PETIT,
    AUCUN
}

int taille_x = 0;
int taille_y = 0;

for (const auto &poly : polygones) {
    auto index = 0;
    auto meilleur_index = -1;
    auto meilleur_agencement = AUCUN;

    for (auto &rectangle : rectangles_libre) {
        if (rectangle.largeur == poly->res_x && rectangle.hauteur == poly->res_y) {
            // extrait rectangle
            if (meilleur_agencement != HAUTEUR_LARGEUR) {
                meilleur_index = index;
                meilleur_agencement = HAUTEUR_LARGEUR;
            }
        }
        else if (rectangle.largeur == poly->res_x && rectangle.hauteur > poly->res_y) {
            //
            if (meilleur_agencement != HAUTEUR_LARGEUR) {
                meilleur_index = index;
                meilleur_agencement = LARGEUR;
            }
        }
        else if (rectangle.largeur > poly->res_x && rectangle.hauteur == poly->res_y) {
            //
            if (meilleur_agencement != HAUTEUR_LARGEUR) {
                meilleur_index = index;
                meilleur_agencement = HAUTEUR;
            }
        }
        else if (rectangle.largeur > poly->res_x && rectangle.hauteur > poly->res_y) {
            //
            if (meilleur_agencement != HAUTEUR_LARGEUR) {
                meilleur_index = index;
                meilleur_agencement = PLUS_PETIT;
            }
        }

        index++;
    }

    if (meilleur_index == -1) {
        // agrandit l'image
        if (taille_x < taille_y) {
            auto pos_x = taille_x;
            auto pos_y = 0;

            taille_x += poly->res_x;

            Rectangle rect;
            rect.pos_x = pos_x;
            rect.pos_y = poly->res_y;
            rect.taille_x = poly->res_x;
            rect.taille_y = taille_y - poly->res_y;

            rectangles_libre.push_back(rect);
        }
        else  {
            auto pos_x = 0;
            auto pos_y = taille_y;

            taille_y += poly->res_y;

            Rectangle rect;
            rect.pos_x = poly->res_x;
            rect.pos_y = pos_y;
            rect.taille_x = taille_x - poly->res_x;
            rect.taille_y = poly->res_y;

            rectangles_libre.push_back(rect);
        }
    }
    else {
        if (meilleur_agencement == HAUTEUR_LARGEUR) {
            poly->x = rectangles_libre[meilleur_index].pos_x;
            poly->y = rectangles_libre[meilleur_index].pos_y;
            rectangles_libre.erase(rectangles_libre.begin() + meilleur_index);
        }
        else if (meilleur_agencement == LARGEUR) {

        }
        else if (meilleur_agencement == HAUTEUR) {

        }
        else if (meilleur_agencement == PLUS_PETIT) {

        }
    }
}
