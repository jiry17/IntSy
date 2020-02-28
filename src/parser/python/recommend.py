import sys
from sexp import sexp as sexpParser
from enum import Enum
import os
import pprint as pp

temp_rule = {"=b": "="}

def preprocessWithTempRule(expr):
    if type(expr) == str:
        if expr in temp_rule:
            return temp_rule[expr]
        else:
            return expr
    elif type(expr) == tuple:
        return expr
    elif type(expr) == list:
        result = []
        for sub_expr in expr:
            result.append(preprocessWithTempRule(sub_expr))
        return result
    else:
        myAssert(False, "Unknown expression")

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

def parseValue(s: str):
    if s in ["false", "true"]:
        return "Bool", s
    try:
        value = int(s)
        return "Int", value
    except ValueError:
        pass
    return "String", s

def parseExample(line):
    assert line[-1] == '\n'
    line = line[: -1]
    inp, oup = line.split(" => ")
    inp_list = inp[1:-1].split(",")
    inp_list = list(map(lambda s: parseValue(s), inp_list))
    oup = parseValue(oup)
    return [inp_list, oup]

def examplesFromFile(inp_file):
    with open(inp_file, "r") as inp:
        lines = inp.readlines()
    result = list(map(lambda line: parseExample(line), lines))
    return result

def getSublist(bmExpr, f):
    return list(filter(lambda x: f(x[0]), bmExpr))

def exprToString(expr):
    if type(expr) == tuple:
        return str(expr[1])
    elif type(expr) == str:
        return expr
    elif type(expr) == list:
        result = []
        for sub_expr in expr:
            result.append(exprToString(sub_expr))
        return "(" + " ".join(result) + ")"

def prepareNewExpr(bmExpr, example_list):
    syn_info = getSublist(bmExpr, lambda x: x == "synth-fun")[0]
    remain_expr = getSublist(bmExpr, lambda x: x != "constraint" and x != "check-synth")
    syn_name = syn_info[1]
    for inp, oup in example_list:
        func_call = [syn_name]
        func_call.extend(inp)
        remain_expr.append(["constraint", ["=", func_call, oup]])
    remain_expr.append(["check-synth"])
    #pp.pprint(remain_expr)
    with open(output_file + ".mid", "w") as oup:
        for sub_expr in remain_expr:
            oup.write(exprToString(sub_expr) + "\n")

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

def callEusolver(path):
    solver_inp = current_path + output_file + ".mid"
    solver_oup = current_path + output_file + ".out"
    os.system("cd " + path + ";" + "./eusolver " + solver_inp + " > " + solver_oup)
    bmExpr = sexpFromFile(solver_oup)
    return bmExpr[0]


def callEuphony(path):
    solver_inp = current_path + output_file + ".mid"
    solver_oup = current_path + output_file + ".out"
    commend = "cd " + path + "; . bin/setenv; ./bin/run_circuit " + solver_inp + " > " + solver_oup
    os.system(commend)
    with open(solver_oup, "r") as inp:
        lines = inp.readlines()
        while lines[0][0] != "(":
            lines = lines[1:]
        expr = "".join(list(map(lambda x: x[:-1] if x[-1] == "\n" else x, lines)))
        bmExpr = sexpFromString(expr)
    return bmExpr

def callSolver(path):
    if "eusolver" in path:
        return callEusolver(path)
    elif "euphony" in path:
        return callEuphony(path)

def changeParam(program, param_list):
    if type(program) == str:
        for i, (param_name, param_type) in enumerate(param_list):
            if param_name == program:
                return ("Param", i)
        return program
    if type(program) == tuple:
        return program
    if type(program) == list:
        result = []
        for sub_program in program:
            result.append(changeParam(sub_program, param_list))
        return result
    myAssert(False, "Unknown program type")

def getPureProgram(result):
    param_list = result[2]
    return changeParam(result[4], param_list)

if __name__ == "__main__":
    assert len(sys.argv) == 5
    _, file_name, benchmark_type, output_file, solver_path = sys.argv
    current_path = os.getcwd() + "/"
    bmExpr = sexpFromFile(file_name)
    bmExpr = preprocessWithTempRule(bmExpr)
    example_list = examplesFromFile(output_file + ".in")
    prepareNewExpr(bmExpr, example_list)
    result = callSolver(solver_path)
    result = getPureProgram(result)
    with open(output_file + ".res", "w") as oup:
        printList(result, oup)
    os.system("rm " + output_file + ".mid")
    os.system("rm " + output_file + ".out")
    finish(ResultStatus.OK)
