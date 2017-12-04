import sys

def generate(cond_accesses, body_accesses):
    vec_tag = 'vec[f64]'
    vecs = ['v' + str(i) for i in range(body_accesses)]
    vecs_tagged = [v + ': ' + vec_tag for v in vecs]
    params = ','.join(vecs_tagged)
    params = '|' + params + '|'
    start = params + 'result(for(map(filter(zip('
    start += ','.join(vecs) + '),'
    cond_lambda = '|x| x.$0 >= 19940101.0 && x.$0 < 19950101.0 &&'
    cond_lambda +=  '&&'.join(['x.$' + str(i+1) + ' > 9.0' for i in range(cond_accesses-1)]) + '),'
    body_lambda = '|x| ' + '+'.join(['x.$' + str(i+1) for i in range(body_accesses-1)]) + '),'
    end = 'merger[f64,+],|b,i,x| merge(b,x)))'

    return start + cond_lambda + body_lambda + end

def main():
    if len(sys.argv) != 3:
        print 'incorrect number of arguments'
        return

    cond = int(sys.argv[1])
    body = int(sys.argv[2])
    print generate(cond, body)
        
if __name__=='__main__':
    main()