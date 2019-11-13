# ifndef SLIDINGWINDOW_H
# define SLIDINGWINDOW_H

using namespace std;

class slidingwindow
{
    unsigned int m_size;
    int m_front;
    int m_rear;
    int *m_data = NULL;
    int *m_data_len = NULL;
    char *m_buffer = NULL;
    
    public:
        slidingwindow();

        slidingwindow(unsigned size);

        ~slidingwindow();

        int init(unsigned size);

        bool isEmpty();

        int push(int seq, int data_len, char *data, int index);

        int slide(char *output_data);

        int topup();
};
# endif