#include <iostream>
#include <fstream>
#include <cstdint>

#pragma pack(push, 1)
struct BMPHeader {
    uint16_t fileType{0x4D42};
    uint32_t fileSize{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offsetData{0};
};

struct BMPInfoHeader {
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bitCount{0};
    uint32_t compression{0};
    uint32_t sizeImage{0};
    int32_t xPixelsPerMeter{0};
    int32_t yPixelsPerMeter{0};
    uint32_t colorsUsed{0};
    uint32_t colorsImportant{0};
};
#pragma pack(pop)

struct Pixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

class BMP {
public:
    BMP(const std::string &filename);
    ~BMP();
    void Save(const std::string &filename);
    void Rotate90();
    void RotateCounter90();
    void GaussianFilter();

private:
    BMPHeader header;
    BMPInfoHeader infoHeader;
    Pixel** data;
    bool Memory(int height, int width);
    void FreeMemory(int height);
};

bool BMP::Memory(int height,int width) {
    data = new (std::nothrow) Pixel*[height];
    if (!data) return false;

    for (int i = 0; i < height; ++i) {
        data[i] = new (std::nothrow) Pixel[width];
        if (!data[i]) {
            for (int j = 0; j < i; ++j) {
                delete[] data[j];
            }
            delete[] data;
            data = nullptr;
            return false;
        }
    }
    return true;
}

void BMP::FreeMemory(int height) {
    if (data) {
        for (int i = 0; i < height; ++i) {
            delete[] data[i];
        }
        delete[] data;
        data = nullptr;
    }
}


BMP::BMP(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Error open file.");
    }

    file.read(reinterpret_cast<char *>(&header), sizeof(header));
    if (header.fileType != 0x4D42) {
        throw std::runtime_error("File format is not BMP.");
    }

    file.read(reinterpret_cast<char *>(&infoHeader), sizeof(infoHeader));

    infoHeader.height = std::abs(infoHeader.height);
    infoHeader.width = std::abs(infoHeader.width);

    if (infoHeader.width == 0 || infoHeader.height == 0) {
        throw std::runtime_error("unexceptable file size.");
    }
    file.seekg(header.offsetData, file.beg);


    Memory(infoHeader.height, infoHeader.width);


    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            file.read(reinterpret_cast<char *>(&data[i][j]), sizeof(Pixel));
            if (!file) {
                throw std::runtime_error("Error file read.");
            }
        }
    }

    file.close();
}


BMP::~BMP() {
    FreeMemory(infoHeader.height);
}


void BMP::Save(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("File save error.");
    }

    int byte = infoHeader.height * infoHeader.width * 4;

    std::cout << "File " << filename << " use " << byte << " bytes." << std::endl;

    file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    file.write(reinterpret_cast<const char *>(&infoHeader), sizeof(infoHeader));

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            file.write(reinterpret_cast<const char *>(&data[i][j]), sizeof(Pixel));
        }
    }
    file.close();
}




void BMP::Rotate90() {
    Pixel** rotatedData = new Pixel*[infoHeader.width];
    for (int i = 0; i < infoHeader.width; ++i) {
        rotatedData[i] = new Pixel[infoHeader.height];
    }

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[j][infoHeader.height - i - 1] = data[i][j];
        }
    }

    FreeMemory(infoHeader.height);

    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}


void BMP::RotateCounter90() {
    Pixel** rotatedData = new Pixel*[infoHeader.width];
    for (int i = 0; i < infoHeader.width; ++i) {
        rotatedData[i] = new Pixel[infoHeader.height];
    }

    for (int i = 0; i < infoHeader.height; ++i) {
        for (int j = 0; j < infoHeader.width; ++j) {
            rotatedData[infoHeader.width - j - 1][i] = data[i][j];
        }
    }

    FreeMemory(infoHeader.height);

    data = rotatedData;
    std::swap(infoHeader.width, infoHeader.height);
}

void BMP::GaussianFilter() {
    float kernel[3][3] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    Pixel** tempData = new Pixel*[infoHeader.height];
    for (int i = 0; i < infoHeader.height; ++i) {
        tempData[i] = new Pixel[infoHeader.width];
    }

    for (int y = 0; y < infoHeader.height; ++y) {
        for (int x = 0; x < infoHeader.width; ++x) {
            float sumRed = 0, sumGreen = 0, sumBlue = 0;


            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int nx = x + kx;
                    int ny = y + ky;

                    if (nx < 0) nx = 0;
                    if (nx >= infoHeader.width) nx = infoHeader.width - 1;
                    if (ny < 0) ny = 0;
                    if (ny >= infoHeader.height) ny = infoHeader.height - 1;


                    sumRed   += data[ny][nx].red * kernel[ky + 1][kx + 1];
                    sumGreen += data[ny][nx].green * kernel[ky + 1][kx + 1];
                    sumBlue  += data[ny][nx].blue * kernel[ky + 1][kx + 1];
                }
            }


            tempData[y][x].red = static_cast<uint8_t>((sumRed < 0) ? 0 : ((sumRed > 255) ? 255 : sumRed));
            tempData[y][x].green = static_cast<uint8_t>((sumGreen < 0) ? 0 : ((sumGreen > 255) ? 255 : sumGreen));
            tempData[y][x].blue = static_cast<uint8_t>((sumBlue < 0) ? 0 : ((sumBlue > 255) ? 255 : sumBlue));
        }
    }


    for (int y = 0; y < infoHeader.height; ++y) {
        for (int x = 0; x < infoHeader.width; ++x) {
            data[y][x] = tempData[y][x];
        }
    }

    for (int i = 0; i < infoHeader.height; ++i) {
        delete[] tempData[i];
    }
    delete[] tempData;
}



int main() {
    try {
        BMP image("yard.bmp");

        BMP imageCounterClockwise("yard.bmp");

        image.Rotate90();
        image.Save("output_clockwise.bmp");

        imageCounterClockwise.RotateCounter90();
        imageCounterClockwise.Save("output_counterclockwise.bmp");


        image.GaussianFilter();
        image.Save("output_gaussian.bmp");

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
