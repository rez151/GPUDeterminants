
void swap(long* m, int order, int* index, int row1, int row2){
    int aux;
    aux=index[row1];
    index[row1]=index[row2];
    index[row2]=aux;
    if(row1!=row2) index[order]*=-1;
}

long get(long* m, int order, int* index, int row, int column){
    return m[index[row]*order+column];
}

void set(long* m, int order,  int* index, int row, int column, long value){
    m[index[row]*order+column]=value;
}

void subtract( long* m, int order,  int* index, int row1, int row2, long factor){
    for(int i=0; i<order; i++){
        long v=get(m,order,index,row2,i) - factor*get(m,order,index,row1,i);
        set(m,order,index,row2,i,v);
    }
}

int pivotIndex(long* m, int order, int* index, int diag){
    int result= -1;
    long pivot=LONG_MAX;
    for(int i=diag; i<order; i++){
        long pivotCandidate=abs(get(m, order,index, i,diag));
        if(pivotCandidate!=0 && pivotCandidate<pivot){
            pivot=pivotCandidate;
            result=i;
        }
    }
    return result;
}

int decrease(long* m, int order, int* index, int diag){
    int result=0;
    int pIdx=pivotIndex(m, order,index, diag);
    if(pIdx==-1) return result;
    long pivot;
    pivot=get(m,order,index,pIdx,diag);
    swap(m,order,index,diag,pIdx);

    for(int i=diag+1; i<order; i++){
        long factor = get(m,order,index,i,diag)/pivot;
        if(factor!=0){
            result=1;
            subtract(m,order,index,diag,i,factor);
        }
    }
    return result;
}

void eliminate(long* m, int order,  int* index, int diag){
    while(0!=decrease(m,order,index,diag)){
        ;
    }
}

void triangulate(long* m, int order,  int* index){
    for(int i=0; i<order; i++){
        eliminate(m,order,index,i);
    }
}

long determinant(long* m, int order, int* index){
    triangulate(m,order,index);
    long result=1;
    for(int i=0; i<order; i++){
        result*=get(m,order,index,i,i);
    }
    return index[order]*result;
}

__global long* getMatrix( __global long* ms, int order, int position){
    int offset = order*order * position;
    return ms+offset;
}

__kernel void determinants( __global long* ms, __global long* output){
    long arr[ORDER*ORDER];
    int cell=get_global_id(0);
    //int order=ORDER;
    int index[ORDER+1];
    for(int i=0; i<ORDER;i++){
        index[i]=i;
    }
    index[ORDER]=1;
    //long* arr[ORDER*ORDER];
    //event_t copyevents[1];
    //copyevents[0] = async_work_group_copy(arr,ms+cell*ORDER*ORDER, sizeof(long)*ORDER*ORDER, NULL);
    //wait_group_events(1,copyevents);
    __global long* m=getMatrix(ms, ORDER, cell);
    //TODO
    //long arr[ORDER*ORDER];
    int i,j;
    for(i = 0; i<ORDER; i++){
        for(j=0;j<ORDER;j++){
            arr[i*ORDER+j] = m[i*ORDER+j];
        }
    }
    //TODO
    /*
    event_t e = async_work_group_copy(arr, m, ORDER*ORDER, 0);
    wait_group_events(1, &e);*/
    output[cell] =  determinant(arr, ORDER, index);
}