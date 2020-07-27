//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

bool Scope::IsGlobalEvalBlockScope() const
{
    return this->scopeType == ScopeType_GlobalEvalBlock;
}

bool Scope::IsBlockScope(FuncInfo *funcInfo)
{
    return this != funcInfo->GetBodyScope() && this != funcInfo->GetParamScope();
}

int Scope::AddScopeSlot()
{
    int slot = scopeSlotCount++;
    if (scopeSlotCount == Js::ScopeSlots::MaxEncodedSlotCount)
    {
        this->GetEnclosingFunc()->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("TooManySlots")));
    }
    return slot;
}

void Scope::ForceAllSymbolNonLocalReference(ByteCodeGenerator *byteCodeGenerator)
{
    this->ForEachSymbol([this, byteCodeGenerator](Symbol *const sym)
    {
        if (!sym->IsArguments() && !sym->IsSpecialSymbol())
        {
            sym->SetHasNonLocalReference();
            byteCodeGenerator->ProcessCapturedSym(sym);
            this->GetFunc()->SetHasLocalInClosure(true);
        }
    });
}

bool Scope::IsEmpty() const
{
    return Count() == 0;
}

void Scope::SetIsObject()
{
    if (this->isObject)
    {
        return;
    }

    this->isObject = true;

    // We might set the scope to be object after we have process the symbol
    // (e.g. "With" scope referencing a symbol in an outer scope).
    // If we have func assignment, we need to mark the function to not do stack nested function
    // as these are now assigned to a scope object.
    FuncInfo * funcInfo = this->GetFunc();
    if (funcInfo && !funcInfo->HasMaybeEscapedNestedFunc())
    {
        this->ForEachSymbolUntil([funcInfo](Symbol * const sym)
        {
            if (sym->GetHasFuncAssignment())
            {
                funcInfo->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("DelayedObjectScopeAssignment")));
                return true;
            }
            return false;
        });
    }

    // If the scope is split (there exists a body and param scope), then it is required that the
    // body and param scope are marked as both requiring either a scope object or a scope slot.
    if ((this->GetScopeType() == ScopeType_FunctionBody || this->GetScopeType() == ScopeType_Parameter)
        && funcInfo && !funcInfo->IsBodyAndParamScopeMerged())
    {
        // The scope is split and one of the scopes (param or body) is being set 
        // as an object, therefore set both the param and body scopes as objects.
        Assert(funcInfo->paramScope);
        funcInfo->paramScope->SetIsObject();

        Assert(funcInfo->bodyScope);
        funcInfo->bodyScope->SetIsObject();
    }
}

void Scope::MergeParamAndBodyScopes(ParseNodeFnc *pnodeScope)
{
    Assert(pnodeScope->funcInfo);
    Scope *paramScope = pnodeScope->pnodeScopes->scope;
    Scope *bodyScope = pnodeScope->pnodeBodyScope->scope;

    if (paramScope->Count() == 0)
    {
        return;
    }

    bodyScope->scopeSlotCount = paramScope->scopeSlotCount;
    paramScope->ForEachSymbol([&](Symbol * sym)
    {
        bodyScope->AddNewSymbol(sym);
    });

    if (paramScope->GetIsObject())
    {
        bodyScope->SetIsObject();
    }
    if (paramScope->GetMustInstantiate())
    {
        bodyScope->SetMustInstantiate(true);
    }
    if (paramScope->GetHasOwnLocalInClosure())
    {
        bodyScope->SetHasOwnLocalInClosure(true);
    }
}

void Scope::RemoveParamScope(ParseNodeFnc *pnodeScope)
{
    Assert(pnodeScope->funcInfo);
    Scope *paramScope = pnodeScope->pnodeScopes->scope;
    Scope *bodyScope = pnodeScope->pnodeBodyScope->scope;

    // Once the scopes are merged, there's no reason to instantiate the param scope.
    paramScope->SetMustInstantiate(false);

    paramScope->m_count = 0;
    paramScope->scopeSlotCount = 0;
    paramScope->m_symList = nullptr;
    // Remove the parameter scope from the scope chain.

    bodyScope->SetEnclosingScope(paramScope->GetEnclosingScope());
}
