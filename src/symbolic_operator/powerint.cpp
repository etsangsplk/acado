/*
 *    This file is part of ACADO Toolkit.
 *
 *    ACADO Toolkit -- A Toolkit for Automatic Control and Dynamic Optimization.
 *    Copyright (C) 2008-2014 by Boris Houska, Hans Joachim Ferreau,
 *    Milan Vukov, Rien Quirynen, KU Leuven.
 *    Developed within the Optimization in Engineering Center (OPTEC)
 *    under supervision of Moritz Diehl. All rights reserved.
 *
 *    ACADO Toolkit is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    ACADO Toolkit is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with ACADO Toolkit; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */



/**
*    \file src/symbolic_operator/powerint.cpp
*    \author Boris Houska, Hans Joachim Ferreau
*    \date 2008
*/


#include <acado/utils/acado_utils.hpp>
#include <acado/symbolic_operator/symbolic_operator.hpp>



BEGIN_NAMESPACE_ACADO




Power_Int::Power_Int() : SmoothOperator( )
{
}

Power_Int::Power_Int( const SharedOperator &_argument, const int &_exponent ) : SmoothOperator( )
{
    argument          = _argument                        ;
    exponent          = _exponent                        ;
    argument_result   = (double*)calloc(1,sizeof(double));
    dargument_result  = (double*)calloc(1,sizeof(double));
    bufferSize        = 1                                ;
    curvature         = CT_UNKNOWN                       ;
    monotonicity      = MT_UNKNOWN                       ;
}


Power_Int::Power_Int( const Power_Int &arg ){

    int run1;

    bufferSize       = arg.bufferSize;
    exponent         = arg.exponent;
    argument         = arg.argument;
    dargument        = arg.dargument;

    argument_result  = (double*)calloc(bufferSize,sizeof(double));
    dargument_result = (double*)calloc(bufferSize,sizeof(double));

    for( run1 = 0; run1 < bufferSize; run1++ ){

        argument_result[run1] = arg.argument_result[run1];
       dargument_result[run1] = arg.dargument_result[run1];

    }
    curvature         = arg.curvature   ;
    monotonicity      = arg.monotonicity;

    derivative  = arg.derivative;
    derivative2 = arg.derivative2;
}


Power_Int::~Power_Int(){

    free(  argument_result );
    free( dargument_result );
}

Power_Int& Power_Int::operator=( const Power_Int &arg ){

    if( this != &arg ){

        free(  argument_result );
        free( dargument_result );

        argument = arg.argument;

        exponent          = arg.exponent                       ;
        bufferSize        = arg.bufferSize                     ;
        argument_result   = (double*)calloc(bufferSize,sizeof(double))  ;
        dargument_result  = (double*)calloc(bufferSize,sizeof(double))  ;

        curvature         = arg.curvature   ;
        monotonicity      = arg.monotonicity;
    }
    return *this;
}


returnValue Power_Int::evaluate( int number, double *x, double *result ){

    if( number >= bufferSize ){
        bufferSize += number;
        argument_result  = (double*)realloc( argument_result,bufferSize*sizeof(double));
        dargument_result = (double*)realloc(dargument_result,bufferSize*sizeof(double));
    }
    argument->evaluate( number, x , &argument_result[number] );

    result[0] = pow( argument_result[number], exponent );

    return SUCCESSFUL_RETURN;
}


returnValue Power_Int::evaluate( EvaluationBase *x ){
 
    x->powerInt(*argument,exponent);
    return SUCCESSFUL_RETURN;
}


SharedOperator Power_Int::differentiate( int index ){

  if( exponent == 0 ){
    return SharedOperator( new DoubleConstant( 0.0 , NE_ZERO ));
  }
  dargument = argument->differentiate( index );
  return myProd( derivative, dargument );
}


SharedOperator Power_Int::AD_forward( int dim,
                                   VariableType *varType,
                                   int *component,
                                   SharedOperator *seed,
                                   std::vector<SharedOperator> &newIS ){

	dargument = argument->AD_forward(dim,varType,component,seed,newIS);
	return myProd( derivative, dargument );
}



returnValue Power_Int::AD_backward( int           dim      , /**< number of directions  */
                                        VariableType *varType  , /**< the variable types    */
                                        int          *component, /**< and their components  */
                                        SharedOperator     &seed     , /**< the backward seed     */
                                        SharedOperator    *df       , /**< the result            */
                                        std::vector<SharedOperator> &newIS  /**< the new IS-pointer    */ ){

  
    SharedOperator ttt = myProd( derivative, seed );
    argument->AD_backward( dim, varType, component, ttt, df, newIS );
    return SUCCESSFUL_RETURN;
}



returnValue Power_Int::AD_symmetric( int            dim       , /**< number of directions  */
                                        VariableType  *varType   , /**< the variable types    */
                                        int           *component , /**< and their components  */
                                        SharedOperator  &l         , /**< the backward seed     */
                                        SharedOperator  *S         , /**< forward seed matrix   */
                                        int            dimS      , /**< dimension of forward seed             */
                                        SharedOperator     *dfS       , /**< first order foward result             */
                                        SharedOperator     *ldf       , /**< first order backward result           */
                                        SharedOperator     *H         , /**< upper trianglular part of the Hessian */
                                      std::vector<SharedOperator> &newLIS , /**< the new LIS-pointer   */
                                      std::vector<SharedOperator> &newSIS , /**< the new SIS-pointer   */
                                      std::vector<SharedOperator> &newHIS   /**< the new HIS-pointer   */ ){
  
    SharedOperator tmp  = convert2TreeProjection( derivative  );
    SharedOperator tmp2 = convert2TreeProjection( derivative2 );

    return ADsymCommon( argument, tmp, tmp2, dim, varType, component, l, S, dimS, dfS,
			 ldf, H, newLIS, newSIS, newHIS );
}


returnValue Power_Int::initDerivative() {

	if( derivative != 0 ) {
		return SUCCESSFUL_RETURN;
	}

	SharedOperator powerTmp = myPowerInt( argument, exponent-1 );
	SharedOperator expTmp = SharedOperator( new DoubleConstant( (double) exponent, NE_NEITHER_ONE_NOR_ZERO ));

	derivative = convert2TreeProjection(myProd( expTmp, powerTmp ));

	SharedOperator powerTmp2 = myPowerInt( argument, exponent-2 );
	SharedOperator expTmp2 = SharedOperator( new DoubleConstant( (double) exponent-1, NE_NEITHER_ONE_NOR_ZERO ));
	SharedOperator prodTmp = myProd( expTmp, expTmp2 );

	derivative2 = convert2TreeProjection(myProd( prodTmp, powerTmp2 ));

	return argument->initDerivative();
}



SharedOperator Power_Int::substitute( int index, const SharedOperator &sub ){

    return SharedOperator( new Power_Int( argument->substitute( index , sub ), exponent ));

}


NeutralElement Power_Int::isOneOrZero() const{ return NE_NEITHER_ONE_NOR_ZERO; }


BooleanType Power_Int::isDependingOn( VariableType var ) const{

    return argument->isDependingOn(var);
}


BooleanType Power_Int::isDependingOn( int dim,
                                        VariableType *varType,
                                        int *component,
                                        BooleanType   *implicit_dep ){

    if( exponent == 0 ){
        return BT_FALSE;
    }

    return argument->isDependingOn( dim, varType, component, implicit_dep );
}


BooleanType Power_Int::isLinearIn( int dim,
                                     VariableType *varType,
                                     int *component,
                                     BooleanType   *implicit_dep ){

    if( exponent == 0 ){
        return BT_TRUE;
    }

    if( exponent == 1 && argument->isLinearIn( dim, varType, component, implicit_dep ) == BT_TRUE ){
        return BT_TRUE;
    }

    return BT_FALSE;
}


BooleanType Power_Int::isPolynomialIn( int dim,
                                         VariableType *varType,
                                         int *component,
                                         BooleanType   *implicit_dep ){

    if(  argument->isPolynomialIn( dim, varType, component, implicit_dep )    == BT_TRUE &&
         exponent >= 0 ){
        return BT_TRUE;
    }

    return BT_FALSE;
}


BooleanType Power_Int::isRationalIn( int dim,
                                       VariableType *varType,
                                       int *component,
                                       BooleanType   *implicit_dep ){

    if(  argument->isRationalIn( dim, varType, component, implicit_dep ) == BT_TRUE ){
        return BT_TRUE;
    }

    return BT_FALSE;
}


MonotonicityType Power_Int::getMonotonicity( ){

    if( monotonicity != MT_UNKNOWN )  return monotonicity;

    const MonotonicityType m = argument->getMonotonicity();

    if( m == MT_CONSTANT )  return MT_CONSTANT;

    // if exponent is even:
    if( fabs( ceil( ((double) exponent)/2.0 - EPS ) - ((double) exponent)/2.0 ) < 10.0*EPS ){

        if( exponent == 0 ) return MT_CONSTANT;

        return MT_NONMONOTONIC;
    }
    else{

        if( exponent > 0  )  return m;
        return MT_NONMONOTONIC;
    }

    return MT_NONMONOTONIC;
}


CurvatureType Power_Int::getCurvature( ){

    if( curvature != CT_UNKNOWN )  return curvature;

    const CurvatureType cc = argument->getCurvature();

    if( cc == CT_CONSTANT )  return CT_CONSTANT;

    // if exponent is even:
    if( fabs( ceil( ((double) exponent)/2.0 - EPS ) - ((double) exponent)/2.0 ) < 10.0*EPS ){

        if( exponent  < 0   ) return CT_NEITHER_CONVEX_NOR_CONCAVE;
        if( exponent == 0   ) return CT_CONSTANT;
        if( cc == CT_AFFINE ) return CT_CONVEX  ;

        return CT_NEITHER_CONVEX_NOR_CONCAVE;
    }
    else{

        if( exponent == 1 ) return cc;
        return CT_NEITHER_CONVEX_NOR_CONCAVE;
    }

    return CT_NEITHER_CONVEX_NOR_CONCAVE;
}


returnValue Power_Int::setMonotonicity( MonotonicityType monotonicity_ ){

    monotonicity = monotonicity_;
    return SUCCESSFUL_RETURN;
}


returnValue Power_Int::setCurvature( CurvatureType curvature_ ){

    curvature = curvature_;
    return SUCCESSFUL_RETURN;
}


returnValue Power_Int::AD_forward( int number, double *x, double *seed,
                                   double *f, double *df ){

    if( number >= bufferSize ){
        bufferSize += number;
        argument_result  = (double*)realloc( argument_result,bufferSize*sizeof(double));
        dargument_result = (double*)realloc(dargument_result,bufferSize*sizeof(double));
    }
    argument->AD_forward( number, x, seed, &argument_result[number],
                                  &dargument_result[number] );

      f[0] =  pow( argument_result[number],exponent );
     df[0] =  exponent*pow( argument_result[number],exponent-1 )*dargument_result[number];

     return SUCCESSFUL_RETURN;
}



returnValue Power_Int::AD_forward( int number, double *seed, double *df ){


    argument->AD_forward( number, seed, &dargument_result[number] );

     df[0] =  exponent*pow( argument_result[number],exponent-1 )*dargument_result[number];

     return SUCCESSFUL_RETURN;
}


returnValue Power_Int::AD_backward( int number, double seed, double *df ){
    return argument->AD_backward( number, exponent*pow( argument_result[number],
                                  exponent-1 )*seed, df );
}


returnValue Power_Int::AD_forward2( int number, double *seed, double *dseed,
                                    double *df, double *ddf ){

    double      ddargument_result;
    double      dargument_result2;

    argument->AD_forward2( number, seed, dseed, &dargument_result2, &ddargument_result);

    const double nn = exponent*pow( argument_result[number],exponent-1 );
     df[0] = nn*dargument_result2;
    ddf[0] = nn*ddargument_result + exponent*(exponent-1)*dargument_result[number]
                                  * dargument_result2*
                                    pow( argument_result[number],exponent-2 )   ;

    return SUCCESSFUL_RETURN;
}


returnValue Power_Int::AD_backward2( int number, double seed1, double seed2,
                               double *df, double *ddf ){

    const double nn = exponent*pow(argument_result[number],exponent-1);

    argument->AD_backward2( number,
                            seed1*nn,
                            seed2*nn +
                            seed1*exponent*(exponent-1)
                                 *pow(argument_result[number],exponent-2)
                                 *dargument_result[number],
                            df, ddf );

    return SUCCESSFUL_RETURN;
}


std::ostream& Power_Int::print( std::ostream &stream ) const{

  
  
     // TODO: this type of symplification should be implemented somewhere else...
  
// 	if ( argument->getName() == ON_POWER ) 
// 	{
// 		Power* dummy = (Power*)argument;
// 
// 		if ( acadoIsEqual( dummy->argument2->getValue(),1.0/((double)exponent) ) == BT_TRUE )
// 		{
// 			return stream << "(" << *(dummy->argument1) << ")";
// 		}
// 	}

	if ( exponent == 1 )
	{
	    return stream << "(" << *argument << ")";
	}
	else
	{
		VariableType dummy1;
		int dummy2;

		if ( ( exponent == 2 ) && ( argument->isVariable( dummy1,dummy2 ) == BT_TRUE ) )
		{
			return stream << "((" << *argument << ")*(" << *argument << "))";
		}
		else
		{
			return stream << "(pow(" << *argument << "," << exponent << "))";
		}
	}
}



returnValue Power_Int::clearBuffer(){

    if( bufferSize > 1 ){
        bufferSize = 1;
        argument_result  = (double*)realloc( argument_result,bufferSize*sizeof(double));
        dargument_result = (double*)realloc(dargument_result,bufferSize*sizeof(double));
    }

    return SUCCESSFUL_RETURN;
}


returnValue Power_Int::enumerateVariables( SymbolicIndexList *indexList ){

    return argument->enumerateVariables( indexList );
}


//
// PROTECTED MEMBER FUNCTIONS:
// ---------------------------


OperatorName Power_Int::getName(){

    return ON_POWER_INT;
}

BooleanType Power_Int::isVariable( VariableType &varType, int &component ) const
{
    return BT_FALSE;
}

returnValue Power_Int::loadIndices( SymbolicIndexList *indexList ){

    return argument->loadIndices( indexList );
}


BooleanType Power_Int::isSymbolic() const{

    if( argument->isSymbolic() == BT_FALSE ) return BT_FALSE;
    return BT_TRUE;
}

returnValue Power_Int::setVariableExportName(	const VariableType &_type,
												const std::vector< std::string >& _name
												)
{
	argument->setVariableExportName(_type, _name);

	return Operator::setVariableExportName(_type, _name);
}

CLOSE_NAMESPACE_ACADO

// end of file.
