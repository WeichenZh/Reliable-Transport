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
    m_rear = m_front + m_size;
}   

slidingwindow::slidingwindow(unsigned size)
{   
    m_size = size;
    m_front = 0;
    m_rear = m_front + m_size;
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
    m_rear = m_front + m_size;
    m_data = new (std::nothrow) int[size];
    m_buffer = new (std::nothrow) char[size*buffersize];
    for (int i=0; i<size;i++)
        m_data[i]=-1;
    memset(m_buffer, 0, size*buffersize);

    return 0;  
}

bool slidingwindow::isEmpty()
{   
    return m_front == m_rear;
}   

bool slidingwindow::isFull() 
{   
    return m_front == (m_rear + 1)%m_size;
}  

void slidingwindow::push(int seq, char *data, int index)throw(bad_exception)
{
    if(isFull() || index > m_size || index < 0)
    {
        throw bad_exception();
    }
    m_data[(m_front+index)%m_size] = seq;
    strcpy(m_buffer+((m_front+index)%m_size*buffersize), data);
}

int slidingwindow::slide(char *output_data) throw(bad_exception)
{
    if(isEmpty())
    {
        throw bad_exception();
    }
    int tmp = m_data[m_front];
    strcpy(output_data, m_buffer+m_front*buffersize);
    m_front = (m_front + 1)%m_size;
    m_rear = (m_rear + 1)%m_size;
    m_data[m_rear] = -1;
    memset(m_buffer+m_rear*buffersize, 0, buffersize);
    return tmp;
}
