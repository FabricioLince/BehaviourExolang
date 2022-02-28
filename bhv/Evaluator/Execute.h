#ifndef EVAL_EXECUTE_H
#define EVAL_EXECUTE_H


#include "Evaluator.h"
#include <algorithm>

void Evaluator::evaluateArg(Node* arg, Datatable* data, Datatable* childData) {
  //std::cout << arg << "\n";
  if (arg->name == "assign") {
    assign(arg, data, childData);
  }
  else {
    Variable value = evaluate(arg, data);
    Variable args = childData->getLocal("_a"); // temporary, so it doesn't overwrite the current context's args
    if (args.type != Variable::LIST) {
      args = Variable(Variable::VarList());
    }
    args.list.push_back(value);
    childData->setLocal("_a", args);
  }
}

void Evaluator::evaluateArgs(Tree* tree, Datatable* data, Datatable* childData) {
  Tree* argTree = tree->subTree(1);
  if (argTree->children.size() > 0) {
    argTree = argTree->subTree(0);
    //std::cout << "args:\n";
    evaluateArg(argTree->children.at(0), childData, childData);

    Tree* args = argTree->subTree("args");
    for (unsigned int i = 0; i < args->children.size(); ++i) {
      evaluateArg(args->children.at(i), data, childData);
    }
    
    Variable argList = childData->getLocal("_a");
    if (argList.type == Variable::LIST) {
      std::vector<std::string> names = std::vector<std::string>({"a", "b", "c", "d", "e"});
      for (unsigned int i = 0; i < std::min(names.size(), argList.list.size()); ++i) {
        childData->setLocal(names.at(i), argList.list.at(i));
      }
    }
    if (argList.type != Variable::NIL) {
      childData->setLocal("al", argList);
    }
    childData->removeLocal("_a");
  }
}

Variable Evaluator::execute(Tree* tree, Datatable* data) {
  //std::cout << tree << "\n";
  
  Variable bt = evaluate(tree->children.at(0), data);
  switch (bt.type) {
    case Variable::STRING:
      return bt.string.size() > 0;
      break;
    case Variable::NUMBER:
      return int(bt.number) != 0;
      break;
    case Variable::LIST:
      return bt.list.size() > 0;
      break;
    case Variable::NODE:
    case Variable::CFUNC:
    {
      Datatable* childData = bt.context;
      
      if (bt.context == NULL) {
        childData = data->makeOrphan(); /// create new Datatable for this execution
      }
      
      childData->context = data;
      
      evaluateArgs(tree, data, childData);
      
      Variable result = justExecute(bt, childData);
      
      childData->context = NULL;
      
      return result;
    }
  }
  
  return bt.toBool();
}


Variable Evaluator::executeChildTree(Node* node, Datatable* data) {
  return evaluate(node, data);
}
Variable Evaluator::executeCFunc(Variable cfunc, Datatable* data) {
  return (*cfunc.cfunc)(data);
}

Variable Evaluator::executeAny(Variable nodeOrFunc, Datatable* data, Variable arg, Variable b) {
  Datatable* childData = nodeOrFunc.context;
  if (childData == NULL) {
    childData = data->makeOrphan();
  }
  
  childData->setLocal("a", arg);
  
  if (b.type != Variable::NIL) {
    childData->setLocal("b", b);
  }
  
  childData->context = data;
  
  return justExecute(nodeOrFunc, childData);
}

Variable Evaluator::justExecute(Variable nodeOrFunc, Datatable* data) {
  Variable r = Variable();
  if (nodeOrFunc.type == Variable::NODE) {
    if (nodeOrFunc.invert) {
      return !executeChildTree(nodeOrFunc.node, data).toBool();
    }
    else
      return executeChildTree(nodeOrFunc.node, data);
  }
  else if (nodeOrFunc.type == Variable::CFUNC) {
    if (nodeOrFunc.invert) {
      return !executeCFunc(nodeOrFunc, data).toBool();
    }
    else
      return executeCFunc(nodeOrFunc, data);
  }
  return r;
}

Variable Evaluator::getTree(Tree* tree, Datatable* data) {
  //std::cout << tree << "\n";
  Datatable* treeData = NULL;
  if (tree->subTree(0)->children.size() > 0) {
    // not orphan node
    treeData = data->makeChild();
  }
  else {
    // is orphan node
    treeData = NULL;
  }
  return Variable(tree->children.at(1), treeData);
}

#endif