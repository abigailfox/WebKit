/*
 * Copyright (C) 2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CSSCalcExpressionNode.h"

namespace WebCore {

enum CSSValueID : uint16_t;

class CSSCalcSymbolNode final : public CSSCalcExpressionNode {
public:
    static Ref<CSSCalcSymbolNode> create(CSSValueID, CSSUnitType);

    String customCSSText() const;

private:
    CSSCalcSymbolNode(CSSValueID, CSSUnitType);

    // CSSCalcExpressionNode overrides
    bool isResolvable() const final;
    bool isZero() const final;
    std::unique_ptr<CalcExpressionNode> createCalcExpression(const CSSToLengthConversionData&) const final;
    double doubleValue(CSSUnitType, const CSSCalcSymbolTable&) const final;
    double computeLengthPx(const CSSToLengthConversionData&) const final;
    bool equals(const CSSCalcExpressionNode&) const final;
    Type type() const final;
    CSSUnitType primitiveType() const final;
    void collectComputedStyleDependencies(ComputedStyleDependencies&) const final;
    void dump(TextStream&) const final;

    CSSValueID m_symbol;
    CSSUnitType m_unitType;
};

}

SPECIALIZE_TYPE_TRAITS_CSSCALCEXPRESSION_NODE(CSSCalcSymbolNode, type() == WebCore::CSSCalcExpressionNode::Type::CssCalcSymbol)
