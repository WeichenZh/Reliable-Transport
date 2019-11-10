# ifndef SLIDINGWINDOW_H
# define SLIDINGWINDOW_H

using namespace std;

class slidingwindow
{
    unsigned int m_size;
    int m_front;
    int m_rear;
    int*  m_data = NULL;
    char *m_buffer = NULL;
    
    public:
        slidingwindow();

        slidingwindow(unsigned size);

        ~slidingwindow();

        int init(unsigned size);

        bool isEmpty();

        bool isFull() ;

        void push(int seq, char *data, int index);

        int slide(char *output_data);

        int topup();
};
# endif