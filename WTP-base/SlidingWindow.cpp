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
    if(m_data_len)
        delete m_data_len;
    if(m_buffer)
        delete m_buffer;
}   

int slidingwindow::init(unsigned size)
{
    m_size = size;
    m_front = 0;
    m_rear = m_front + m_size-1;
    m_data = new (std::nothrow) int[size];
    m_data_len = new (std::nothrow) int[size];
    m_buffer = new (std::nothrow) char[size*buffersize];
    for (int i=0; i<size;i++)
    {
        m_data[i]=-1;
        m_data_len[i] = 0;
    }
    ///memset(m_data_len, 0, size*sizeof(int));
    memset(m_buffer, 0, size*buffersize);
    return 0;  
}

bool slidingwindow::isEmpty()
{   
    return m_front == m_rear;
}   

int slidingwindow::push(int seq, int data_len, char *data, int index)
{
    // cout << "seq in s:" << seq << endl;
    // cout << "index in s:" << index << endl << endl;
    if(index >= m_size || index < 0)
        return -1;

    m_data[(m_front+index)%m_size] = seq;
    m_data_len[(m_front+index)%m_size] = data_len;
    memcpy(m_buffer+((m_front+index)%m_size*buffersize), data, data_len);
    // cout << data_len << endl;
    // cout << data <<endl;
    return 0;
}

int slidingwindow::slide(char *output_data)
{
    if(isEmpty())
    {
        error("Error: window is empty\n");
    }

    int front_data_len;
    memcpy(output_data, m_buffer+m_front*buffersize, m_data_len[m_front]);
    front_data_len = m_data_len[m_front];

    m_front = (m_front + 1)%m_size;
    m_rear = (m_rear + 1)%m_size;

    m_data[m_rear] = -1;
    m_data_len[m_rear] = 0;
    memset(m_buffer+m_rear*buffersize, 0, buffersize);

    return front_data_len;
}

// return the first number of the window, but not pop out.
int slidingwindow::topup()
{
    return m_data[m_front];
}
