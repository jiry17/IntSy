import sys
from sexp import sexp as sexpParser
from enum import Enum
import pprint as pp

class ResultStatus(Enum):
    OK = 0
    WA = 1

def finish(status : ResultStatus, message = ""):
    with open(output_file + ".log", "w") as log:
        log.write(str(status.value) + "\n")
        log.write(message + "\n")
    exit(status.value)

def myAssert(condition, message):
    if not condition:
        print(message)
        finish(ResultStatus.WA, message)

def stripComments(bmFile):
    noComments = '('
    for line in bmFile:
        line = line.split(';', 1)[0]
        noComments += line
    return noComments + ')'

def sexpFromString(value):
    return sexpParser.parseString(value, parseAll = True).asList()[0]

def sexpFromFile(benchmarkFileName):
    try:
        benchmarkFile = open(benchmarkFileName)
    except:
        # print('File not found: %s' % benchmarkFileName)
        return None

    bm = stripComments(benchmarkFile)
    bmExpr = sexpFromString(bm)
    benchmarkFile.close()
    return bmExpr

def getSublist(bmExpr, name):
    return list(filter(lambda x: x[0] == name, bmExpr))

def getName(func_info): return func_info[1]
def getParamType(func_info): return list(map(lambda x: x[1], func_info[2]))
def getReturnType(func_info): return func_info[3]
def getSpec(func_info): return func_info[4]

def substituteParam(func, param_list):
    if type(func) == str:
        for (i, (name, _)) in enumerate(param_list):
            if name == func:
                return "Param", i
        return func
    elif type(func) == list:
        return list(map(lambda x: substituteParam(x, param_list), func))
    else:
        return func

def printList(info, oup):
    if type(info) == list:
        oup.write(str(len(info)) + "\n")
        for item in info:
            printList(item, oup)
    else:
        oup.write("0\n")
        if type(info) == str:
            oup.write(info + "\n")
        elif type(info) == tuple:
            myAssert(type(info[0]) == str, "The first item's type of tuples should be string")
            myAssert(info[0] in {"Param", "Int", "Bool", "String"}, "Unknown tuple " + str(info))
            oup.write("#" + info[0][:1] + str(info[1]) + "\n")
        else:
            myAssert(False, "Unknown data type in printList: " + str(type(info)))

def getOracleFromExpr(spec, param_list, var_map, result_type):
    '''
    (define-fun Spec ((k1 Bool) (k2 Bool) (k3 Bool) (r1 Bool) (r2 Bool) (r3 Bool) (r4 Bool)) Bool
    )
    '''
    oracle_spec = ["define-fun", "oracle"]
    type_list = []
    for param_name in param_list:
        type_list.append([param_name, var_map[param_name]])
    oracle_spec.append(type_list)
    oracle_spec.append(result_type)
    oracle_spec.append(spec)
    oracle_call = ["oracle"]
    for param_name in param_list:
        oracle_call.append(param_name)
    return oracle_spec, oracle_call


def extractOracle(bmExpr):
    myAssert(len(getSublist(bmExpr, "constraint")) == 1, "#constraint should be 1")
    myAssert(len(getSublist(bmExpr, "synth-fun")) == 1, "#synth-fun should be 1")
    constraint = getSublist(bmExpr, "constraint")[0]
    syn_info = getSublist(bmExpr, "synth-fun")[0]
    result = []
    var_map = {}
    for sub_expr in bmExpr:
        if not sub_expr[0] == "constraint":
            result.append(sub_expr)
            if sub_expr[0] == "declare-var":
                var_map[sub_expr[1]] = sub_expr[2]
    constraint = constraint[1]
    myAssert(constraint[0] == "=", "Constraint should be equal")
    left_expr, right_expr = constraint[1], constraint[2]
    if type(right_expr) == list and right_expr[0] == getName(syn_info):
        left_expr, right_expr = right_expr, left_expr
    myAssert(left_expr[0] == getName(syn_info), "Constraint should contain the target function")
    param_list = left_expr[1:]
    for param in param_list:
        myAssert(type(param) == str and param in var_map, "Parameters should be variables")
    oracle_spec, oracle_call = getOracleFromExpr(right_expr, param_list, var_map, getReturnType(syn_info))
    result.append(oracle_spec)
    result.append(["constraint", ["=", left_expr, oracle_call]])
    return result


def parseOracleBenchmark(bmExpr, output_file):
    '''
    (define-fun Spec ((k1 Bool) (k2 Bool) (k3 Bool) (r1 Bool) (r2 Bool) (r3 Bool) (r4 Bool)) Bool
    )
    '''
    if len(getSublist(bmExpr, "define-fun")) == 0:
        bmExpr = extractOracle(bmExpr)
    myAssert(len(getSublist(bmExpr, "constraint")) == 1, "#constraints should be 1")
    myAssert(len(getSublist(bmExpr, "define-fun")) == 1, "#define-fun should be 1")
    myAssert(len(getSublist(bmExpr, "synth-fun")) == 1, "#synth-fun should be 1")
    constraint = getSublist(bmExpr, "constraint")[0]
    oracle_info = getSublist(bmExpr, "define-fun")[0]
    syn_info = getSublist(bmExpr, "synth-fun")[0]
    myAssert(getReturnType(oracle_info) == getReturnType(syn_info) and getParamType(oracle_info) == getParamType(syn_info),
             "def-fun should be the oracle")
    myAssert({getName(oracle_info), getName(syn_info)} == {constraint[1][1][0], constraint[1][2][0]} and constraint[1][0] == "=",
             "Constraint should be {def_fun = syn_fun}")

    oracle_spec = substituteParam(getSpec(oracle_info), oracle_info[2])
    syn_spec = substituteParam(getSpec(syn_info), syn_info[2])

    '''   
    Type info
    oracle
    syn
    '''

    with open(output_file + ".res", "w") as oup:
        printList([getParamType(syn_info), oracle_spec, syn_spec], oup)
    finish(ResultStatus.OK)

def parseExample(constraint, syn_info):
    syn_name = getName(syn_info)
    myAssert(len(constraint) == 2, "Invalid constraint")
    constraint = constraint[1]
    myAssert(constraint[0] == "=", "Invalid constraint")
    l_expr, r_expr = constraint[1], constraint[2]
    if type(l_expr) == tuple:
        l_expr, r_expr = r_expr, l_expr
    myAssert(type(r_expr) == tuple and r_expr[0] == "String", "Invalid right expression")
    myAssert(type(l_expr) == list and l_expr[0] == getName(syn_info), "Invalid left expression")
    myAssert(len(l_expr) == len(getParamType(syn_info)) + 1, "Invalid function call")
    return l_expr[1:], r_expr

def replaceConstants(string_spec, syn_info):
    assert(0)

def parseStringBenchmark(bmExpr, output_file):
    myAssert(len(getSublist(bmExpr, "define-fun")) <= 1, "#define-fun should be at most 1")
    myAssert(len(getSublist(bmExpr, "synth-fun")) == 1, "#synth-fun should be 1")
    constraint_list = getSublist(bmExpr, "constraint")
    syn_info = getSublist(bmExpr, "synth-fun")[0]
    example_list = []
    string_spec = '((Start String (S (cons Start Start) (replace S Sc Sc) (delete S Sc))) ' \
                  '(Sc String (P string_cons)) (S String (Sc (substr P I I) )) ' \
                  '(I Int (C (move Ind C))) (C Int (int_cons)) (Ind Int ((index P S I) (len P))) (P String (param)))'
    string_spec = sexpFromString(string_spec)
    inp_type_list = getParamType(syn_info)
    for inp_type in inp_type_list:
        myAssert(inp_type == "String", "The type of the inputs should be String")

    for constraint in constraint_list:
        inp, oup = parseExample(constraint, syn_info)
        is_valid = True
        for inp_value in inp:
            if len(inp_value) == 0:
                is_valid = False
        if not is_valid:
            continue
        example_list.append([inp, oup])

    max_length = 0
    for example in example_list:
        for inp_value in example[0]:
            max_length = max(max_length, len(inp_value[1]))
    int_cons_list = []
    for i in range(-max_length, max_length):
        #if abs(i) <= 6 or max_length - i <= 6:
        int_cons_list.append(("Int", i))
    string_cons_list = []
    string_cons_dict = {}

    def collectStringCons(benchmark_spec):
        if type(benchmark_spec) == tuple:
            if benchmark_spec[0] == "String" and len(benchmark_spec[1]) > 0:
                string_cons_dict[benchmark_spec[1]] = 0
            return
        elif type(benchmark_spec) == str:
            return
        elif type(benchmark_spec) == list:
            for sub_spec in benchmark_spec:
                collectStringCons(sub_spec)
            return
        else:
            myAssert(False, "Unknown spec type")

    collectStringCons(getSpec(syn_info))

    length_limit = 4
    num_limit = 3
    for length in range(1, length_limit):
        occur_count = {}
        for inp_list, _ in example_list:
            current_set = {}
            for ty, inp in inp_list:
                if len(inp) < length:
                    continue
                for l in range(len(inp) - length):
                    current = inp[l: l+length]
                    current_set[current] = 1
            for occur_string in current_set.keys():
                if occur_string not in occur_count:
                    occur_count[occur_string] = 0
                occur_count[occur_string] += 1
        current_list = []
        for occur_string, value in occur_count.items():
            current_list.append([value, occur_string])
        current_list = sorted(current_list, reverse=True)
        if len(current_list) > num_limit:
            current_list = current_list[:num_limit]
        for value, string in current_list:
            if value * 2 <= len(example_list):
                continue
            string_cons_dict[string] = 0
    for string in string_cons_dict.keys():
        string_cons_list.append(("String", string))
    #pp.pprint(string_cons_list)

    def replaceConstants(spec):
        if type(spec) == list:
            result = []
            for sub_spec in spec:
                if sub_spec == "string_cons":
                    result.extend(string_cons_list)
                elif sub_spec == "int_cons":
                    result.extend(int_cons_list)
                elif sub_spec == "param":
                    for i in range(len(inp_type_list)):
                        result.append(("Param", i))
                else:
                    result.append(replaceConstants(sub_spec))
            return result
        else:
            return spec

    string_spec = replaceConstants(string_spec)
    with open(output_file + ".res", "w") as oup:
        printList([getParamType(syn_info), getReturnType(syn_info), example_list, string_spec, string_cons_list], oup)
    finish(ResultStatus.OK)


if __name__ == "__main__":
    myAssert(len(sys.argv) == 4, "python3 main.py $file_name $benchmark_type $output_file")
    file_name = sys.argv[1]
    benchmark_name = sys.argv[2]
    output_file = sys.argv[3]
    result = sexpFromFile(file_name)
    if benchmark_name in ["circuit", "int"]:
        parseOracleBenchmark(result, output_file)
    elif benchmark_name == "string":
        parseStringBenchmark(result, output_file)
    else:
        myAssert(False, "Invalid benchmark type")