// Fill out your copyright notice in the Description page of Project Settings.


#include "K2Node_ForEachMap.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "EditorCategoryUtils.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_GetArrayItem.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"
#include "KismetCompiler.h"
#include "Kismet/BlueprintMapLibrary.h"
#include "Kismet/KismetMathLibrary.h"


#define LOCTEXT_NAMESPACE "K2Node_ForEachMap"


namespace Constants
{
	const FName MapPinName = "Map";
	const FName KeyPinName = "Key";
	const FName ValuePinName = "Value";
}

void UK2Node_ForEachMap::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// input params
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	
	FCreatePinParams MapPinParams;
	MapPinParams.ContainerType = EPinContainerType::Map;
	FEdGraphTerminalType ValueType;
	ValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;
	MapPinParams.ValueTerminalType = ValueType;
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, Constants::MapPinName, MapPinParams);

	// output params
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Loop);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, Constants::KeyPinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, Constants::ValuePinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Completed);
}

void UK2Node_ForEachMap::PostReconstructNode()
{
	Super::PostReconstructNode();

	UEdGraphPin* MapPin = GetMapPin();
	if (MapPin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* LinkedPin = MapPin->LinkedTo[0];
		
		PropagateToPin(MapPin, LinkedPin->PinType, true);
		PropagateToPin(GetKeyPin(), LinkedPin->PinType);
		PropagateToPin(GetValuePin(), FEdGraphPinType::GetPinTypeForTerminalType(LinkedPin->PinType.PinValueType));
	}
}

void UK2Node_ForEachMap::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin == GetMapPin())
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
			
			PropagateToPin(Pin, LinkedPin->PinType, true);
			PropagateToPin(GetKeyPin(), LinkedPin->PinType);
			PropagateToPin(GetValuePin(), FEdGraphPinType::GetPinTypeForTerminalType(LinkedPin->PinType.PinValueType));

			GetGraph()->NotifyNodeChanged(this);
		}
		else
		{
			ResetPin(GetMapPin());
			ResetPin(GetKeyPin());
			ResetPin(GetValuePin());
		}
	}
}

void UK2Node_ForEachMap::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static const FName GetLengthName = "Map_Length";
	static const FName GetLengthInputName = "TargetMap";
	static const FName GetKeysName = "Map_Keys";
	static const FName GetKeysMapParamName = "TargetMap";
	static const FName GetKeysReturnValueName = "Keys";

	static const FName MapFindName = "Map_Find";
	static const FName MapFindMapParamName = "TargetMap";
	static const FName MapFindKeyParamName = "Key";
	static const FName MapFindReturnValueName = "Value";

	static const FName LessThanName = "Less_IntInt";
	static const FName AdditionName = "Add_IntInt";

	UK2Node_CallFunction* GetLengthNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetLengthNode->FunctionReference.SetExternalMember(GetLengthName, UBlueprintMapLibrary::StaticClass());
	GetLengthNode->AllocateDefaultPins();
	UEdGraphPin* GetLengthResultPin = GetLengthNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);

	UK2Node_TemporaryVariable* KeysVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	FEdGraphPinType MapPinType = GetMapPin()->PinType;
	KeysVarNode->VariableType = {MapPinType.PinCategory, MapPinType.PinSubCategory, MapPinType.PinSubCategoryObject.Get(),
		EPinContainerType::Array, false, FEdGraphTerminalType()};
	KeysVarNode->AllocateDefaultPins();
	UEdGraphPin* KeysVarPin = KeysVarNode->GetVariablePin();

	UK2Node_CallFunction* GetKeysNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetKeysNode->FunctionReference.SetExternalMember(GetKeysName, UBlueprintMapLibrary::StaticClass());
	GetKeysNode->AllocateDefaultPins();
	UEdGraphPin* GetKeysExecPin = GetKeysNode->GetExecPin();
	UEdGraphPin* GetKeysInputPin = GetKeysNode->FindPinChecked(GetKeysMapParamName);
	UEdGraphPin* GetKeysOutputPin = GetKeysNode->FindPinChecked(GetKeysReturnValueName);
	UEdGraphPin* GetKeysThenPin = GetKeysNode->GetThenPin();
	
	UK2Node_AssignmentStatement* KeysAssignmentNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	KeysAssignmentNode->AllocateDefaultPins();
	UEdGraphPin* KeysAssignmentExecPin = KeysAssignmentNode->GetExecPin();
	UEdGraphPin* KeysAssignmentVarPin = KeysAssignmentNode->GetVariablePin();
	UEdGraphPin* KeysAssignmentValuePin = KeysAssignmentNode->GetValuePin();
	UEdGraphPin* KeysAssignmentThenPin = KeysAssignmentNode->GetThenPin();
	
	UK2Node_TemporaryVariable* CounterVarNode = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	CounterVarNode->VariableType = {UEdGraphSchema_K2::PC_Int, NAME_None, nullptr,
		EPinContainerType::None, false, FEdGraphTerminalType()};
	CounterVarNode->AllocateDefaultPins();
	UEdGraphPin* CounterVarPin = CounterVarNode->GetVariablePin();

	UK2Node_CallFunction* LessThanNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	LessThanNode->FunctionReference.SetExternalMember(LessThanName, UKismetMathLibrary::StaticClass());
	LessThanNode->AllocateDefaultPins();
	UEdGraphPin* LessThanFirstPin = LessThanNode->FindPinChecked(TEXT("A"));
	UEdGraphPin* LessThanSecondPin = LessThanNode->FindPinChecked(TEXT("B"));
	UEdGraphPin* LessThanResultPin = LessThanNode->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);

	UK2Node_IfThenElse* BranchNode = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	BranchNode->AllocateDefaultPins();
	UEdGraphPin* BranchExecPin = BranchNode->GetExecPin();
	UEdGraphPin* BranchConditionPin = BranchNode->GetConditionPin();
	UEdGraphPin* BranchThenPin = BranchNode->GetThenPin();
	UEdGraphPin* BranchElsePin = BranchNode->GetElsePin();

	UK2Node_ExecutionSequence* SequenceNode = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(this, SourceGraph);
	SequenceNode->AllocateDefaultPins();
	UEdGraphPin* SequenceExecPin = SequenceNode->GetExecPin();
	UEdGraphPin* SequenceFirstPin = SequenceNode->GetThenPinGivenIndex(0);
	UEdGraphPin* SequenceSecondPin = SequenceNode->GetThenPinGivenIndex(1);

	UK2Node_CallFunction* AdditionNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	AdditionNode->FunctionReference.SetExternalMember(AdditionName, UKismetMathLibrary::StaticClass());
	AdditionNode->AllocateDefaultPins();
	UEdGraphPin* AdditionFirstPin = AdditionNode->FindPinChecked(TEXT("A"));
	UEdGraphPin* AdditionSecondPin = AdditionNode->FindPinChecked(TEXT("B"));
	AdditionSecondPin->DefaultValue = "1";
	UEdGraphPin* AdditionResultPin = AdditionNode->GetReturnValuePin();

	UK2Node_AssignmentStatement* CounterAssignmentNode = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	CounterAssignmentNode->AllocateDefaultPins();
	UEdGraphPin* AssignmentExecPin = CounterAssignmentNode->GetExecPin();
	UEdGraphPin* AssignmentVariablePin = CounterAssignmentNode->GetVariablePin();
	UEdGraphPin* AssignmentValuePin = CounterAssignmentNode->GetValuePin();
	UEdGraphPin* AssignmentThenPin = CounterAssignmentNode->GetThenPin();

	UK2Node_GetArrayItem* GetKeyNode = CompilerContext.SpawnIntermediateNode<UK2Node_GetArrayItem>(this, SourceGraph);
	GetKeyNode->AllocateDefaultPins();
	UEdGraphPin* GetKeyArrayPin = GetKeyNode->GetTargetArrayPin();
	UEdGraphPin* GetKeyIndexPin = GetKeyNode->GetIndexPin();
	UEdGraphPin* GetKeyValuePin = GetKeyNode->GetResultPin();

	UK2Node_CallFunction* GetMapValueNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetMapValueNode->FunctionReference.SetExternalMember(MapFindName, UBlueprintMapLibrary::StaticClass());
	GetMapValueNode->AllocateDefaultPins();
	UEdGraphPin* GetMapValueMapPin = GetMapValueNode->FindPinChecked(MapFindMapParamName);
	UEdGraphPin* GetMapValueKeyPin = GetMapValueNode->FindPinChecked(MapFindKeyParamName);
	UEdGraphPin* GetMapValueValuePin = GetMapValueNode->FindPinChecked(MapFindReturnValueName);
	
	// let's go
	{
		// initialize keys array
		MovePins(CompilerContext, GetExecPin(), GetKeysExecPin);
		CopyPins(CompilerContext, GetMapPin(), GetKeysInputPin);
		ConnectPins(GetKeysThenPin, KeysAssignmentExecPin);
		ConnectPins(KeysVarPin, KeysAssignmentVarPin);
		ConnectPins(GetKeysOutputPin, KeysAssignmentValuePin);
		
		// connect input map to length node
		CopyPins(CompilerContext, GetMapPin(), GetLengthNode->FindPinChecked(GetLengthInputName));

		// plug length and counter into comparison node
		ConnectPins(CounterVarPin, LessThanFirstPin);
		ConnectPins(GetLengthResultPin, LessThanSecondPin);

		// connect exec path to branch
		ConnectPins(KeysAssignmentThenPin, BranchExecPin);
		
		// plug comparison node result into branch
		ConnectPins(LessThanResultPin, BranchConditionPin);

		// hook up completed execution path to branch node
		MovePins(CompilerContext, FindPinChecked(UEdGraphSchema_K2::PN_Completed), BranchElsePin);

		// connect branch and sequence pins 
		ConnectPins(BranchThenPin, SequenceExecPin);

		// loop execution path
		{
			// get key by index
			ConnectPins(GetKeyArrayPin, KeysVarPin);
			ConnectPins(GetKeyIndexPin, CounterVarPin);

			// get value by key
			CopyPins(CompilerContext, GetMapPin(), GetMapValueMapPin);
			ConnectPins(GetKeyValuePin, GetMapValueKeyPin);

			// connect key and value to outputs
			MovePins(CompilerContext, GetKeyPin(), GetKeyValuePin);
			MovePins(CompilerContext, GetValuePin(), GetMapValueValuePin);
			
			// hook up loop execution path to sequence 0 node
			MovePins(CompilerContext, FindPinChecked(UEdGraphSchema_K2::PN_Loop), SequenceFirstPin);
		}
			
		// connect sequence 1 node to go to increment node
		ConnectPins(SequenceSecondPin, AssignmentExecPin);

		// set up assignment and addition
		ConnectPins(CounterVarPin, AdditionFirstPin);
		ConnectPins(CounterVarPin, AssignmentVariablePin);
		ConnectPins(AdditionResultPin, AssignmentValuePin);

		// go back to branch node after assignment
		ConnectPins(AssignmentThenPin, BranchExecPin);
	}

	BreakAllNodeLinks();
}

FText UK2Node_ForEachMap::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("ForEachMap_NodeTitle", "For Each Map");
}

FText UK2Node_ForEachMap::GetTooltipText() const
{
	return LOCTEXT("ForEachMap_Tooltip", "Iterates over map entries");
}

void UK2Node_ForEachMap::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_ForEachMap::GetMenuCategory() const
{
	return LOCTEXT("ForEachMap_MenuCategory", "Utilities|Map");
}

UEdGraphPin* UK2Node_ForEachMap::GetMapPin() const
{
	return FindPinChecked(Constants::MapPinName);
}

UEdGraphPin* UK2Node_ForEachMap::GetKeyPin() const
{
	return FindPinChecked(Constants::KeyPinName);
}

UEdGraphPin* UK2Node_ForEachMap::GetValuePin() const
{
	return FindPinChecked(Constants::ValuePinName);
}

void UK2Node_ForEachMap::PropagateToPin(UEdGraphPin* TargetPin, FEdGraphPinType Type, bool bPropagateValueType)
{
	ensure(TargetPin);
	TargetPin->PinType.PinCategory = Type.PinCategory;
	TargetPin->PinType.PinSubCategory = Type.PinSubCategory;
	TargetPin->PinType.PinSubCategoryObject = Type.PinSubCategoryObject;

	if (bPropagateValueType)
	{
		TargetPin->PinType.PinValueType = Type.PinValueType;
	}
}

void UK2Node_ForEachMap::ResetPin(UEdGraphPin* TargetPin)
{
	TargetPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
	TargetPin->PinType.PinSubCategory = NAME_None;
	TargetPin->PinType.PinSubCategoryObject = nullptr;

	TargetPin->PinType.PinValueType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;  
	TargetPin->PinType.PinValueType.TerminalSubCategory = NAME_None;  
	TargetPin->PinType.PinValueType.TerminalSubCategoryObject = nullptr;  
}

void UK2Node_ForEachMap::ConnectPins(UEdGraphPin* First, UEdGraphPin* Second)
{
	if (!GetSchema()->TryCreateConnection(First, Second))
	{
		UE_LOG(LogTemp, Log, TEXT("Failed to create connection between %s and %s"), First ? *First->GetName() : TEXT("null"),
			Second ? *Second->GetName() : TEXT("null"));
	}
}

void UK2Node_ForEachMap::MovePins(FKismetCompilerContext& Context, UEdGraphPin* Source, UEdGraphPin* Target)
{
	Context.MovePinLinksToIntermediate(*Source, *Target);
}

void UK2Node_ForEachMap::CopyPins(FKismetCompilerContext& Context, UEdGraphPin* Source, UEdGraphPin* Target)
{
	Context.CopyPinLinksToIntermediate(*Source, *Target);
}
