//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/planner/expression_binder.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/exception.hpp"
#include "duckdb/parser/expression/bound_expression.hpp"
#include "duckdb/parser/parsed_expression.hpp"
#include "duckdb/parser/tokens.hpp"
#include "duckdb/planner/expression.hpp"
#include "duckdb/common/unordered_map.hpp"

namespace duckdb {

class Binder;
class ClientContext;
class QueryNode;

class ScalarFunctionCatalogEntry;
class AggregateFunctionCatalogEntry;
class MacroCatalogEntry;
class CatalogEntry;
class SimpleFunction;

struct MacroBinding;

struct BindResult {
	explicit BindResult(string error) : error(error) {
	}
	explicit BindResult(unique_ptr<Expression> expr) : expression(move(expr)) {
	}

	bool HasError() {
		return !error.empty();
	}

	unique_ptr<Expression> expression;
	string error;
};

class ExpressionBinder {
public:
	ExpressionBinder(Binder &binder, ClientContext &context, bool replace_binder = false);
	virtual ~ExpressionBinder();

	//! The target type that should result from the binder. If the result is not of this type, a cast to this type will
	//! be added. Defaults to INVALID.
	LogicalType target_type;

public:
	unique_ptr<Expression> Bind(unique_ptr<ParsedExpression> &expr, LogicalType *result_type = nullptr,
	                            bool root_expression = true);

	//! Returns whether or not any columns have been bound by the expression binder
	bool BoundColumns() {
		return bound_columns;
	}

	string Bind(unique_ptr<ParsedExpression> *expr, idx_t depth, bool root_expression = false);

	// Bind table names to ColumnRefExpressions
	static void BindTableNames(Binder &binder, ParsedExpression &expr,
	                           unordered_map<string, idx_t> *alias_map = nullptr);
	static unique_ptr<Expression> PushCollation(ClientContext &context, unique_ptr<Expression> source,
	                                            const string &collation, bool equality_only = false);
	static void TestCollation(ClientContext &context, const string &collation);

	bool BindCorrelatedColumns(unique_ptr<ParsedExpression> &expr);

	void BindChild(unique_ptr<ParsedExpression> &expr, idx_t depth, string &error);
	static void ExtractCorrelatedExpressions(Binder &binder, Expression &expr);

	static bool ContainsNullType(const LogicalType &type);
	static LogicalType ExchangeNullType(const LogicalType &type);
	static bool ContainsType(const LogicalType &type, LogicalTypeId target);
	static LogicalType ExchangeType(const LogicalType &type, LogicalTypeId target, LogicalType new_type);

	static void ResolveParameterType(LogicalType &type);
	static void ResolveParameterType(unique_ptr<Expression> &expr);

protected:
	virtual BindResult BindExpression(unique_ptr<ParsedExpression> *expr_ptr, idx_t depth,
	                                  bool root_expression = false);

	BindResult BindExpression(BetweenExpression &expr, idx_t depth);
	BindResult BindExpression(CaseExpression &expr, idx_t depth);
	BindResult BindExpression(CollateExpression &expr, idx_t depth);
	BindResult BindExpression(CastExpression &expr, idx_t depth);
	BindResult BindExpression(ColumnRefExpression &expr, idx_t depth);
	BindResult BindExpression(ComparisonExpression &expr, idx_t depth);
	BindResult BindExpression(ConjunctionExpression &expr, idx_t depth);
	BindResult BindExpression(ConstantExpression &expr, idx_t depth);
	BindResult BindExpression(FunctionExpression &expr, idx_t depth, unique_ptr<ParsedExpression> *expr_ptr);
	BindResult BindExpression(LambdaExpression &expr, idx_t depth);
	BindResult BindExpression(OperatorExpression &expr, idx_t depth);
	BindResult BindExpression(ParameterExpression &expr, idx_t depth);
	BindResult BindExpression(PositionalReferenceExpression &ref, idx_t depth);
	BindResult BindExpression(StarExpression &expr, idx_t depth);
	BindResult BindExpression(SubqueryExpression &expr, idx_t depth);

protected:
	virtual BindResult BindFunction(FunctionExpression &expr, ScalarFunctionCatalogEntry *function, idx_t depth);
	virtual BindResult BindAggregate(FunctionExpression &expr, AggregateFunctionCatalogEntry *function, idx_t depth);
	virtual BindResult BindUnnest(FunctionExpression &expr, idx_t depth);
	virtual BindResult BindMacro(FunctionExpression &expr, MacroCatalogEntry *macro, idx_t depth,
	                             unique_ptr<ParsedExpression> *expr_ptr);

	virtual void ReplaceMacroParametersRecursive(unique_ptr<ParsedExpression> &expr);
	virtual void ReplaceMacroParametersRecursive(ParsedExpression &expr, QueryNode &node);
	virtual void ReplaceMacroParametersRecursive(ParsedExpression &expr, TableRef &ref);

	virtual string UnsupportedAggregateMessage();
	virtual string UnsupportedUnnestMessage();

	Binder &binder;
	ClientContext &context;
	ExpressionBinder *stored_binder;
	MacroBinding *macro_binding;
	bool bound_columns = false;
};

} // namespace duckdb
