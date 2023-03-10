#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

// trebuie luate valorile deci cred ca ar fi mai ordonat sa iau o structura pt
// pixelii nostri dragi;
typedef struct rgb
{
    uint64_t r, g, b;
} rgb;

// structura cu datele de baza ale imaginii, aceleasi date care sunt atat extrase
// din fisierul ppm la compresie, cat si introduse la decompresie;
typedef struct imginfo
{
    int width, height;
    int max_val;
    unsigned int size;
    rgb **matrix;
} imginfo;

// memory refresher blue green red = valorile (in ord inversa) rgb ale unui
// pixel. important pt constructie.
// area = dimensiunea unui bloc? in pixeli? deci de ex nodul radacina ar avea?
// 256*256 pixeli? probabil ca asta e relevant pt blocurile de 'aceeasi culoare'
// si colturile.ordinea in care-s scrise e ordinea in care trebuie si 'luate'
// colturile = indici in vector
typedef struct QuadtreeNode
{
    unsigned char blue, green, red;
    uint32_t area;
    int32_t top_left, top_right;
    int32_t bottom_left, bottom_right;
}
__attribute__((packed)) QuadtreeNode;

// structura de noduri de arbore homemade.
typedef struct qtnode
{
    // ar avea sens ca fiecarui nod sa ii fie asociata o matrice de pixeli,
    // acea matrice reprezentand 'bucata' minima in care se poate imparti
    // la decompresie nu o pot folosi, totusi.
    rgb **rgb_val;
    // din nefericire nu stiu unde altundeva sa l introduc, avand in vedere ca eu ar trebui sa il
    // cam stiu dinainte sa il introduc, iar o functie de cautare , cred eu, s-ar scrie ciudat pt ca
    // nu am cum sa apelez de la nodul radacina mereu, o sa l retin de aici. sper
    uint32_t index;
    // si chestiile standard.
    uint64_t blue, green, red;
    uint32_t area;
    struct qtnode *top_left, *top_right;
    struct qtnode *bottom_left, *bottom_right;
} qtnode;

// functia care transforma o imagine intr-o matrice de pixeli.
void get_image(FILE *img, imginfo *image)
{
    int i;
    if (!img)
    {
        fprintf(stderr, "cred ca ai uitat sa bagi fisierul. sau n ai reusit.\n");
        exit(1);
    }
    // se scot lucrurile specifice fisierului ppm in ordinea stiuta
    fscanf(img, "P6\n%d %d %d", &(*image).width, &(*image).height, &(*image).max_val);
    // in contextul in care introduc pixelii intr-o matrice, width reprezinta
    // partea longitudinala, prin urmare, nr de coloane , iar height e nr de
    // linii. nu? nu??
    (*image).size = (*image).width * (*image).height;
    // se aloca spatiu pt matricea de pixeli, aceasta va retine valorile rgb
    // ale pixelilor
    (*image).matrix = (rgb **)calloc((*image).width, sizeof(rgb));
    for (i = 0; i < (*image).width; i++)
        (*image).matrix[i] = (rgb *)calloc((*image).height, sizeof(rgb));
    unsigned char *data = calloc(1, sizeof(unsigned char));
    // important!!
    // daca tin bine minte de cand am scris asta, e pt a scapa de un element de la citire
    // gen un spatiu gol sau asa ceva
    fread(data, 1, 1, img);
    // aici se atribuie valorile rgb in matrice, se citesc cate 3 si se introduc in
    // campul respectiv
    for (i = 0; i < (*image).width; i++)
    {
        for (int j = 0; j < (*image).height; j++)
        {
            unsigned char pixels[3];
            fread(pixels, sizeof(unsigned char), 3, img);
            (*image).matrix[i][j].r = pixels[0];
            (*image).matrix[i][j].g = pixels[1];
            (*image).matrix[i][j].b = pixels[2];
        }
    }
    free(data);
}

// arborificare
// se face in mod recursiv
// initial voiam sa apelez o strucutra care retine coordonatele insa asta nu se
// traduce tocmai bine in contextul unui arbore care o ia in 4 directii si prin
// urmare isi schimba coord dupa caz
// coordonatele se modifica pe parcurs via recursivitate, iar indexul reprezinta
// nr de noduri parcurse si tot creste
void compress(qtnode **node, imginfo info, int height, int width, int x, int y, int limit, int32_t *index)
{
    // pozitiile la un mom dat (evident) nu mai coincid, insa eu continui sa le
    // pun in matrice deci trebuie sa fiu atent la indecsi si diferentele lor
    int i, j, l, k;
    (*node) = calloc(1, sizeof(qtnode));
    (*node)->area = height * width;
    (*node)->rgb_val = (rgb **)calloc(width, sizeof(rgb));

    for (i = 0; i < width; i++)
        (*node)->rgb_val[i] = (rgb *)calloc(height, sizeof(rgb));
    //  ar trebui sa scriu coordonatele x si y ca sa pot lucra asa. unde e 0 e x pt i si y pt j
    //  insa mare atentie! in acelasi timp aici se scrie matricea deci trebuie sa introduc indecsi
    //  pt elementele matricii, ceea ce e foarte important aici.
    //  se retin in functie de punctul de inceput al matricii.
    //  se parcurge matricea mare incepand cu punctele care repr coordonatele, pana la lungimea
    //  aferenta, si se introduc in matricea nodului de pixeli.
    (*node)->index = (*index)++;
    for (i = x, k = 0; i < x + width && k < width; i++, k++)
        for (j = y, l = 0; j < y + height && l < height; j++, l++)
        {
            (*node)->rgb_val[k][l].r = info.matrix[i][j].r;
            (*node)->rgb_val[k][l].g = info.matrix[i][j].g;
            (*node)->rgb_val[k][l].b = info.matrix[i][j].b;
        }
    //  ok avem ce 'defineste' nodul nostru. acum trebuie sa decidem daca continuam asa sau nu.
    uint64_t red_avg = 0, blue_avg = 0, green_avg = 0;
    // formula mediei aritmetice aici
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            red_avg += (uint64_t)((*node)->rgb_val[i][j].r);
            green_avg += (uint64_t)((*node)->rgb_val[i][j].g);
            blue_avg += (uint64_t)((*node)->rgb_val[i][j].b);
        }
    }
    (*node)->red = red_avg / (*node)->area;
    (*node)->green = green_avg / (*node)->area;
    (*node)->blue = blue_avg / (*node)->area;
    unsigned long long int mean;
    mean = 0;
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < height; j++)
        {
            mean = mean + ((*node)->red - (*node)->rgb_val[i][j].r) * ((*node)->red - (*node)->rgb_val[i][j].r) +
                   ((*node)->green - (*node)->rgb_val[i][j].g) * ((*node)->green - (*node)->rgb_val[i][j].g) +
                   ((*node)->blue - (*node)->rgb_val[i][j].b) * ((*node)->blue - (*node)->rgb_val[i][j].b);
        }
    }
    mean /= (3 * (*node)->area);
    // daca se incadreaza in limita admisa, nodul se 'inchide' aici
    if (mean <= (int64_t)(limit))
    {
        (*node)->bottom_left = NULL;
        (*node)->bottom_right = NULL;
        (*node)->top_right = NULL;
        (*node)->top_left = NULL;
        return;
    }
    else
    {
        // altfel ajungem aici, continuam
        // top left , primul nod, o sa aiba aceiasi x si y, acelasi starting point ca si patratul mare, altfel spus
        // top right  o sa fie cu un indice de coloana diferit (deci y) insa acelasi x
        // bottom right se schimba si x si y iar bottom left numai x.
        compress(&(*node)->top_left, info, height / 2, width / 2, x, y, limit, index);
        compress(&(*node)->top_right, info, height / 2, width / 2, x, y + width / 2, limit, index);
        compress(&(*node)->bottom_right, info, height / 2, width / 2, x + height / 2, y + width / 2, limit, index);
        compress(&(*node)->bottom_left, info, height / 2, width / 2, x + height / 2, y, limit, index);
    }
}

// arborificare 2!
// functia care tranforma vectorul de compresie in arbore de compresie.
// alte date = alta abordare
// initial voiam sa aplic functia pe fiecare element,
// insa nu cred pot sa fac asta, deci ramanem pe stilul recursiv.
void arr_to_tree(qtnode **node, QuadtreeNode *arr, int i)
{
    // se atribuie valorile nodului
    (*node) = calloc(1, sizeof(qtnode));
    (*node)->area = arr[i].area;
    (*node)->red = arr[i].red;
    (*node)->green = arr[i].green;
    (*node)->blue = arr[i].blue;
    (*node)->index = i;

    // accesam copiii cand exista;
    if (arr[i].top_left == -1 && arr[i].top_right == -1 && arr[i].bottom_right == -1 && arr[i].bottom_left == -1)
        return;
    else
    {
        arr_to_tree(&(*node)->top_left, arr, arr[i].top_left);
        arr_to_tree(&(*node)->top_right, arr, arr[i].top_right);
        arr_to_tree(&(*node)->bottom_right, arr, arr[i].bottom_right);
        arr_to_tree(&(*node)->bottom_left, arr, arr[i].bottom_left);
    }
}

// functia de decompresie va lua arborele si l va introduce in matricea de pixeli
// efectiv opusul compresiei.
void decompress(qtnode *node, int height, int width, imginfo *info, int x, int y)
{
    // in matrice se vor scrie toate valorile rgb obtinute de pe frunze.
    if (node == NULL)
        return;
    int i, j;
    // se afla matricea 'pe bucati' prin intermediul indecsilor
    if (node->top_left == NULL && node->top_right == NULL && node->bottom_right == NULL && node->bottom_left == NULL)
        for (i = x; i < x + width; i++)
        {
            for (j = y; j < y + height; j++)
            {
                // se introduc datele din nod in matrice
                (*info).matrix[i][j].r = node->red;
                (*info).matrix[i][j].g = node->green;
                (*info).matrix[i][j].b = node->blue;
            }
        }
    // am mai discutat despre coordonatele alese, nu se schimba situatia nici
    else
    {
        decompress(node->top_left, height / 2, width / 2, info, x, y);
        decompress(node->top_right, height / 2, width / 2, info, x, y + width / 2);
        decompress(node->bottom_right, height / 2, width / 2, info, x + width / 2, y + width / 2);
        decompress(node->bottom_left, height / 2, width / 2, info, x + width / 2, y);
    }
}

// functie pt a determina frunzele
int count_leaves(qtnode *node)
{
    // am ales sa parcurg post ordine. mi se parea mai placut la ochi.
    // daca un nod nu are niciun alt sub nod inseamna ca e frunza.
    if (node == NULL)
        return 0;

    int leaf_count = 0;
    if (node->top_left == NULL && node->top_right == NULL && node->bottom_left == NULL && node->bottom_right == NULL)
        leaf_count++;

    leaf_count += count_leaves(node->top_left);
    leaf_count += count_leaves(node->top_right);
    leaf_count += count_leaves(node->bottom_left);
    leaf_count += count_leaves(node->bottom_right);

    return leaf_count;
}

void vectorify(qtnode *node, QuadtreeNode **arr, int32_t *index)
{

    if (node == NULL)
        return;
    // se atribuie valorile binecunoscute din arbore.
    (*arr)[*index].area = node->area;
    (*arr)[*index].red = (unsigned char)(node->red);
    (*arr)[*index].blue = (unsigned char)node->blue;
    (*arr)[*index].green = (unsigned char)node->green;
    // si se verifica daca are copii; in functie de asta se afla indecsii pentru pozitii.
    if (node->top_left != NULL)
        (*arr)[*index].top_left = node->top_left->index;
    else
        (*arr)[*index].top_left = -1;

    if (node->top_right != NULL)
        (*arr)[*index].top_right = node->top_right->index;
    else
        (*arr)[*index].top_right = -1;

    if (node->bottom_right != NULL)
        (*arr)[*index].bottom_right = node->bottom_right->index;
    else
        (*arr)[*index].bottom_right = -1;

    if (node->bottom_left != NULL)
        (*arr)[*index].bottom_left = node->bottom_left->index;
    else
        (*arr)[*index].bottom_left = -1;

    (*index)++;

    // daca inca putem parcurge arborele, continuam
    if (&(*arr)[*index].top_left != NULL)
        vectorify(node->top_left, arr, index);
    if (&(*arr)[*index].top_right != NULL)
        vectorify(node->top_right, arr, index);
    if (&(*arr)[*index].bottom_right != NULL)
        vectorify(node->bottom_right, arr, index);
    if (&(*arr)[*index].bottom_left != NULL)
        vectorify(node->bottom_left, arr, index);
}

void swap(qtnode **a, qtnode **b)
{
    qtnode *tmp = *a;
    *a = *b;
    *b = tmp;
}

// functiile de oglindire sunt identice cu exceptia nodurilor care se schimba
// se apeleaza functia de mai sus pentru a inlocui nodurile intre ele
void h_mirror(qtnode **node)
{
    if ((*node)->top_left != NULL)
    {
        h_mirror(&(*node)->top_left);
        h_mirror(&(*node)->top_right);
        h_mirror(&(*node)->bottom_right);
        h_mirror(&(*node)->bottom_left);
    }

    swap(&(*node)->top_left, &(*node)->top_right);
    swap(&(*node)->bottom_left, &(*node)->bottom_right);
}

void v_mirror(qtnode **node)
{
    if ((*node)->top_left != NULL)
    {
        v_mirror(&(*node)->top_left);
        v_mirror(&(*node)->top_right);
        v_mirror(&(*node)->bottom_right);
        v_mirror(&(*node)->bottom_left);
    }

    swap(&(*node)->top_left, &(*node)->bottom_left);
    swap(&(*node)->top_right, &(*node)->bottom_right);
}

// functie de eliberare
void free_tree(qtnode **node, int size)
{
    int i;
    if (*node == NULL)
        return;
    // gandire similara ca si pana acum, parcurgem pana la frunze

    if ((*node)->top_left != NULL)
        free_tree(&(*node)->top_left, size / 2);
    if ((*node)->top_right != NULL)
        free_tree(&(*node)->top_right, size / 2);
    if ((*node)->bottom_right != NULL)
        free_tree(&(*node)->bottom_right, size / 2);
    if ((*node)->bottom_left != NULL)
        free_tree(&(*node)->bottom_left, size / 2);

    // pentru asta mi am retinut size-ul, la compresie, avand matricea
    // folosita si alocata, trebuia eliberata, insa la decompresie nu.
    if ((*node)->rgb_val != NULL)
    {
        for (i = 0; i < size; i++)
            free((*node)->rgb_val[i]);
        free((*node)->rgb_val);
    }
    // instructiunea asta e la final pt ca vrem sa eliberam de la frunze la
    // radacina.
    free(*node);
}

int main(int argc, char **argv)
{
    int i = 0;
    int32_t n = 0;
    imginfo image;
    QuadtreeNode *arr;
    qtnode *root_node = NULL;
    // verificam argumentele
    if (strstr(argv[1], "-c"))
    {
        FILE *img = fopen(argv[3], "rb");
        FILE *compressed_img = fopen(argv[4], "wb");
        // parsam limita admisa
        int limit = atoi(argv[2]);
        get_image(img, &image);
        int node_count = 0, leaf_count = 0;
        // comprimam + aflam nr de noduri/frunze pt a fi introduse in fisierul de out
        compress(&root_node, image, image.height, image.width, 0, 0, limit, &node_count);
        leaf_count = count_leaves(root_node);

        fwrite(&leaf_count, sizeof(int), 1, compressed_img);
        fwrite(&node_count, sizeof(int), 1, compressed_img);
        // alocam vectorul si l determinam
        arr = calloc(node_count, sizeof(QuadtreeNode));
        vectorify(root_node, &arr, &n);
        // aici se baga in fisier vectorul, tot odata
        fwrite(arr, sizeof(QuadtreeNode), node_count, compressed_img);
        // free urile le fac in fiecare if deoarece lucrez cu altceva de fiecare data.
        fclose(img);
        fclose(compressed_img);
        free_tree(&root_node, image.height);
        // eliberarea matricii
        for (i = 0; i < image.width; i++)
            free(image.matrix[i]);
        free(image.matrix);
        free(arr);
    }
    if (strstr(argv[1], "-d"))
    {
        // aici are loc pe scurt o conversie din fisier .out in fisier .ppm.
        // prin urmare se va extrage (si introduce) informatia din fisierul
        // comprimat in arbore, iar apoi din arbore se va scrie in fisierul .ppm
        // e o dilema aici si anume felul in care voi introduce chestiile
        // comprimate = cele de aceeasi culoare pe o arie mai mare
        // inceputul ar trebui sa semene cu finalul de la compresie.
        // defapt tot ar trebui sa semene cu compresia, e decompresie pana la urma.
        // compresie pe dos
        FILE *compressed_img = fopen(argv[2], "rb");
        FILE *final_img = fopen(argv[3], "wb");
        int i, j;
        int node_count, leaf_count;
        // se citesc datele cunoscute
        fread(&leaf_count, sizeof(int), 1, compressed_img);
        fread(&node_count, sizeof(int), 1, compressed_img);
        // se citeste tot vectorul odata si apoi se converteste in copac
        arr = calloc(node_count, sizeof(QuadtreeNode));
        fread(arr, sizeof(QuadtreeNode), node_count, compressed_img);
        arr_to_tree(&root_node, arr, 0);
        // se atribuie valorile imaginii
        image.size = arr[0].area;
        image.width = sqrt(arr[0].area);
        image.height = image.width;
        image.max_val = 255;
        image.matrix = (rgb **)calloc(image.width, sizeof(rgb));
        for (i = 0; i < image.width; i++)
            image.matrix[i] = (rgb *)calloc(image.height, sizeof(rgb));
        // decompresie apoi introducerea in fisier
        decompress(root_node, image.height, image.width, &image, 0, 0);
        fprintf(final_img, "P6\n%d %d\n%d\n", image.width, image.height, image.max_val);

        for (i = 0; i < image.width; i++)
        {
            for (j = 0; j < image.height; j++)
            {
                fwrite(&image.matrix[i][j].r, sizeof(unsigned char), 1, final_img);
                fwrite(&image.matrix[i][j].g, sizeof(unsigned char), 1, final_img);
                fwrite(&image.matrix[i][j].b, sizeof(unsigned char), 1, final_img);
            }
        }

        fclose(compressed_img);
        fclose(final_img);
        for (i = 0; i < image.width; i++)
            free(image.matrix[i]);
        free(image.matrix);
        free_tree(&root_node, image.height);
        free(arr);
    }
    if (strstr(argv[1], "-m"))
    {
        FILE *img = fopen(argv[4], "rb");
        FILE *mirrored_img = fopen(argv[5], "wb");
        int i, j;

        get_image(img, &image);

        int node_count = 0;
        int limit = atoi(argv[3]);
        compress(&root_node, image, image.height, image.width, 0, 0, limit, &node_count);
        // apel functie inversare.
        if (strchr(argv[2], 'h'))
            h_mirror(&root_node);
        else if (strchr(argv[2], 'v'))
            v_mirror(&root_node);
        // asa dau update la matricea de pixeli
        decompress(root_node, image.height, image.width, &image, 0, 0);
        // si scriu informatia in fisier
        fprintf(mirrored_img, "P6\n%d %d\n%d\n", image.width, image.height, image.max_val);
        for (i = 0; i < image.width; i++)
        {
            for (j = 0; j < image.height; j++)
            {
                fwrite(&image.matrix[i][j].r, sizeof(unsigned char), 1, mirrored_img);
                fwrite(&image.matrix[i][j].g, sizeof(unsigned char), 1, mirrored_img);
                fwrite(&image.matrix[i][j].b, sizeof(unsigned char), 1, mirrored_img);
            }
        }
        fclose(img);
        fclose(mirrored_img);
        for (i = 0; i < image.width; i++)
            free(image.matrix[i]);
        free(image.matrix);
        free_tree(&root_node, image.height);
    }
}
