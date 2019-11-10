# include <iostream>
# include <string.h>
# include "SlidingWindow.h"
# include "./utils.h"

# define buffersize 2000
using namespace std;


slidingwindow::slidingwindow()
{   
    m_size = 0;
    m_front = 0;
    m_rear = 0;
}   

slidingwindow::slidingwindow(unsigned size)
{   
    m_size = size;
    m_front = 0;
    m_rear = m_front + m_size-1;    
}   

slidingwindow::~slidingwindow()
{   
    if(m_data)
        delete m_data;
    if(m_buffer)
        delete m_buffer;
}   

int slidingwindow::init(unsigned size)
{
    m_size = size;
    m_front = 0;
    m_rear = m_front + m_size-1;
    m_data = new (std::nothrow) int[size];
    m_buffer = new (std::nothrow) char[size*buffersize];
    for (int i=0; i<size;i++)
        m_data[i]=-1;
    memset(m_buffer, 0, size*buffersize);
    cout << "slidingwindow initialized: m_front = " << m_front << " m_rear=" << m_rear <<endl;
    return 0;  
}

bool slidingwindow::isEmpty()
{   
    return m_front == m_rear;
}   

// bool slidingwindow::isFull() 
// {   
//     return m_front == (m_rear + 1)%m_size;
// }  

void slidingwindow::push(int seq, char *data, int index)
{
    if(index > m_size || index < 0)
    {
        error("Error: window is full or index is invalid\n");
    }
    m_data[(m_front+index)%m_size] = seq;
    strcpy(m_buffer+((m_front+index)%m_size*buffersize), data);
    // cout << "seq in s:" << seq << endl;
    // cout << "index in s:" << index << endl << endl;
}

int slidingwindow::slide(char *output_data)
{
    if(isEmpty())
    {
        error("Error: window is empty\n");
    }
    int tmp = m_data[m_front];
    strcpy(output_data, m_buffer+m_front*buffersize);
    m_front = (m_front + 1)%m_size;
    m_rear = (m_rear + 1)%m_size;
    m_data[m_rear] = -1;
    memset(m_buffer+m_rear*buffersize, 0, buffersize);

    // cout << "m_front is : " << m_front <<" after slide" <<endl;
    // cout << "m_rear is : " << m_rear << "after " <<endl;
    // cout << endl;
    return tmp;
}

// return the first number of the window, but not pop out.
int slidingwindow::topup()
{
    return m_data[m_front];
}
