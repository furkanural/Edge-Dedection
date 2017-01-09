#include "OpenCV/cv.h"
#include "OpenCV/highgui.h"
#include <stdio.h>
#include <math.h>

/// dosya yolu belirtilen fotografi okuyan ve geri donen kod blogu
IplImage * read_image(char * path){
    return cvLoadImage(path, 0);
}

/// parametre olarak gelen dosyayi belirtilen dizine yazan ve islem sonucunu donen kod blogu
int write_image(char * path, IplImage * image){
    return cvSaveImage(path, image, 0);
}

/// fotograf bilgisi, LoG filtresi ve filtre boyutunu alarak fotografa uygulayan
/// ve yeni fotografi geriye donen kod blogu
IplImage * apply_filter(IplImage * photo, double ** filter, int filter_size){
    double sum = 0; // konvolusyon toplami sonrasinda yeni degeri normalize etmek icin genel toplam
    double new_value; // olusacak yeni renk degeri
    int center; // uygulanacak filtrenin merkez hucre indeksi
    int tempX; // konvolusyon toplami sirasinda kullanilacak gecici x indeksi
    int tempY; // konvolusyon toplami sirasinda kullanilacak gecici y indeksi
    IplImage * new_image; // filtre uygulandiktan sonra ki goruntu
    
    for (int i=0; i < filter_size; i++) {
        for (int j = 0; j < filter_size; j++) {
            sum += filter[i][j];
        }
    }
    
    center = filter_size / 2;
    new_image = cvCloneImage(photo);
    for (int x = center; x < (photo->height - center); x++) {
        for (int y = center; y < (photo->width - center); y++) {
            tempX = x - center;
            tempY = y - center;
            new_value = 0;
            for (int i = 0, tempX=(x - center); i < filter_size; i++, tempX++) {
                for (int j = 0, tempY=(y-center); j < filter_size; j++, tempY++) {
                    new_value += filter[i][j] * cvGet2D(photo, tempX, tempY).val[0];
                }
            }
            cvSet2D(new_image, x, y, cvScalar(ceil(new_value / sum), 0, 0, 0));
        }
    }
    return new_image;
}

/// LoG filtresi uygulanmis fotograf ile orjinal fotograf arasinda ki
/// farki alarak kenar belirleme yapan kod blogu
IplImage * find_edge(IplImage * orjinal, IplImage * smoothed){
    IplImage * new_image; // kenar belirleme sonucunun tutulacagi degisken
    double new_color; // iki fotograf arasinda ki fark alindiktan sonra olusacak yeni renk
    
    new_image = cvCloneImage(orjinal);
    // her piksel icin (filtreli - orjinal)
    for (int i = 0; i < orjinal->height; i++) {
        for (int j = 0; j < orjinal->width; j++) {
            new_color = cvGet2D(smoothed, i, j).val[0] - cvGet2D(orjinal, i, j).val[0];
            cvSet2D(new_image, i, j, cvScalar(ceil(new_color), 0, 0, 0));
        }
    }
    return new_image;
}

/// parametre olarak gonderilen fotografin istenilen hassasiyete gore
/// ortalama degerini hesaplayan ve geriye donen kod blogu
double find_average(IplImage * photo, double epsilon){
    double average = 128.0; // ortalama degeri
    double temp_average=0; // while dongusunde kullanilacak gecici ortalama degeri
    double m1; // G1 grubunun ortalama degeri
    double m2; // G2 grubunun ortalama degeri
    double g1_sum; // G1 grubunun toplam degeri
    double g1_count; // G1 gurununda bulunanan piksel sayisi
    double g2_sum; // G2 grubunun toplam degeri
    double g2_count; // G2 gurununda bulunanan piksel sayisi
    
    while (average - temp_average > epsilon) {
        g1_count = 0;
        g1_sum = 0;
        g2_count = 0;
        g2_sum = 0;
        m1 = 0;
        m2 = 0;
        for (int i=0; i<photo->height; i++) {
            for (int j=0; j < photo->width; j++) {
                if (cvGet2D(photo, i, j).val[0] > average) {
                    g1_sum += cvGet2D(photo, i, j).val[0];
                    g1_count++;
                }else{
                    g2_sum += cvGet2D(photo, i, j).val[0];
                    g2_count++;
                }
            }
        }
        temp_average = average;
        if (g1_count != 0) {
            m1 = g1_sum / g1_count;
        }
        if (g2_count != 0) {
            m2 = g2_sum / g2_count;
        }
        average = 0.5 * (m1 + m2);
    }
    
    return average;
}

/// parametre olarak gelen fotografi isletilen epsilon hassasiyetine gore
/// siyah-beyaz olarak hesaplayan ve geriye donen kod blogu
IplImage * threshold_level(IplImage * photo, double epsilon){
    IplImage * new_image; //hesapalama sonrasi olusacak fotograf
    double average; // fotografta bulunan renk piksellerinin ortalama degeri
    
    average = find_average(photo, epsilon);
    new_image = cvCloneImage(photo);
    
    for (int i = 0; i < photo->height; i++) {
        for (int j = 0; j < photo->width; j++) {
            if (cvGet2D(photo, i, j).val[0] > average) {
                cvSet2D(new_image, i, j, cvScalar(0, 0, 0, 0));
            }else{
                cvSet2D(new_image, i, j, cvScalar(255, 0, 0, 0));
            }
        }
    }
    
    return new_image;
}

/// Laplacian of Gaussian(LoG) filtresini hesaplayan ve geriye matris olarak donen kod blogu
/// hesaplama icin filtre boyutunu islem matrisini ve sigmaˆ2 degerini kullanilmaktadir
double** calculate_filter(int filter_size, double** kernel, int sigma2){
    double n; // Gaussian denkleminin pay kismi
    double d = 2 * sigma2; // Gaussian denkleminin payda kismi
    
    for (int x = -(filter_size/2); x <= (filter_size/2); x++)
    {
        for(int y = -(filter_size/2); y <= (filter_size/2); y++)
        {
            n = x*x + y*y;
            kernel[x + (filter_size/2)][y + (filter_size/2)] = exp( (-n) / d);
        }
    }
    
    return kernel;
}

/// filtre boyutunu ve filtreyi parametre olarak alan ve filtrenin minimun degerini
/// kullanarak filtreyi normalize eden kod blogu
double** normalize_filter(int filter_size, double** kernel){
    double min = kernel[0][0];
    for(int i = 0; i < filter_size; ++i){
        for(int j = 0; j < filter_size; ++j){
            kernel[i][j] = ceil(kernel[i][j] / min);
        }
    }
    
    return kernel;
}

/// filtre boyutunu ve sigmaˆ2 degerini parametre olarak alan
/// ve Gaussian filtresini olusturmak icin gerekli cagrilari yaptiktan sonra
/// filtreyi geriye donen kod blogu
double** create_filter(int filter_size, double sigma2){
    double **kernel; // filtreyi tutacak olan matris degiskeni
    
    //bellek uzerinde yer tahsisi
    kernel = (double **)malloc(filter_size * sizeof(double *));
    for (int i = 0; i < filter_size; i++) {
        kernel[i] = (double *)malloc(filter_size * sizeof(double));
    }
    
    return normalize_filter(filter_size, calculate_filter(filter_size, kernel, sigma2));
}

int main(int argc, const char * argv[]) {
    
    double epsilon = 0.01;
    double sigma2 = 2.0;
    
    IplImage * house;
    IplImage * monarch;
    IplImage * pentagone;
    
    IplImage * house_smoothed_3x3;
    IplImage * house_smoothed_5x5;
    IplImage * monarch_smoothed_3x3;
    IplImage * monarch_smoothed_5x5;
    IplImage * pentagone_smoothed_3x3;
    IplImage * pentagone_smoothed_5x5;
    
    IplImage * house_edges_3x3;
    IplImage * house_edges_5x5;
    IplImage * monarch_edges_3x3;
    IplImage * monarch_edges_5x5;
    IplImage * pentagone_edges_3x3;
    IplImage * pentagone_edges_5x5;
    
    IplImage * house_black_white;
    IplImage * monarch_black_white;
    IplImage * pentagone_black_white;

    double **kernel_3x3;
    double **kernel_5x5;
    
    kernel_3x3 = create_filter(3, sigma2);
    kernel_5x5 = create_filter(5, sigma2);
    
    house = read_image("odev1/house.256.pgm");
    monarch = read_image("odev1/monarch.512.pgm");
    pentagone = read_image("odev1/pentagone.1024.pgm");

    house_smoothed_3x3 = apply_filter(house, kernel_3x3, 3);
    house_smoothed_5x5 = apply_filter(house, kernel_5x5, 5);
    monarch_smoothed_3x3 = apply_filter(monarch, kernel_3x3, 3);
    monarch_smoothed_5x5 = apply_filter(monarch, kernel_5x5, 5);
    pentagone_smoothed_3x3 = apply_filter(pentagone, kernel_3x3, 3);
    pentagone_smoothed_5x5 = apply_filter(pentagone, kernel_5x5, 5);
    
    write_image("odev1/house.256.smoothed.3x3.pgm", house_smoothed_3x3);
    write_image("odev1/house.256.smoothed.5x5.pgm", house_smoothed_5x5);
    write_image("odev1/monarch.512.smoothed.3x3.pgm", monarch_smoothed_3x3);
    write_image("odev1/monarch.512.smoothed.5x5.pgm", monarch_smoothed_5x5);
    write_image("odev1/pentagone.1024.smoothed.3x3.pgm", pentagone_smoothed_3x3);
    write_image("odev1/pentagone.1024.smoothed.5x5.pgm", pentagone_smoothed_5x5);
    
    house_edges_3x3 = find_edge(house, house_smoothed_3x3);
    house_edges_5x5 = find_edge(house, house_smoothed_5x5);
    monarch_edges_3x3 = find_edge(monarch, monarch_smoothed_3x3);
    monarch_edges_5x5 = find_edge(monarch, monarch_smoothed_5x5);
    pentagone_edges_3x3 = find_edge(pentagone, pentagone_smoothed_3x3);
    pentagone_edges_5x5 = find_edge(pentagone, pentagone_smoothed_5x5);
    
    write_image("odev1/house.256.edge.3x3.pgm", house_edges_3x3);
    write_image("odev1/house.256.edge.5x5.pgm", house_edges_5x5);
    write_image("odev1/monarch.512.edge.3x3.pgm", monarch_edges_3x3);
    write_image("odev1/monarch.512.edge.5x5.pgm", monarch_edges_5x5);
    write_image("odev1/pentagone.1024.edge.3x3.pgm", pentagone_edges_3x3);
    write_image("odev1/pentagone.1024.edge.5x5.pgm", pentagone_edges_5x5);
    
    house_black_white = threshold_level(house, epsilon);
    monarch_black_white = threshold_level(monarch, epsilon);
    pentagone_black_white = threshold_level(pentagone, epsilon);
    
    write_image("odev1/house.256.black_white.pgm", house_black_white);
    write_image("odev1/monarch.512.black_white.pgm", monarch_black_white);
    write_image("odev1/pentagone.1024.black_white.pgm", pentagone_black_white);
    
    
    return 0;
}
