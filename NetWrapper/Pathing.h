#pragma once


using namespace System;
using namespace System::Collections::Generic;


namespace LodNative{
	generic <typename TNode, typename TData>
		ref class Pathing{
		public:
			ref class Result{
			public:
				property TData LinkData;
				property TNode From;
				property TNode To;
				Result(TData data, TNode f, TNode t)
				{
					LinkData = data;
					From = f;
					To = t;
				}
			};
		private:
			ref class Step{
			public:
				property TNode Position;
				property TData Link;
				property Step^ Parent;
				static Step^ CreateFirst(TNode start_position){
					Step^ first = gcnew Step();
					first->Position = start_position;
					return first;
				}
			private:
				Step(){}
				List<Result^>^ getResultVector(int current_size){
					if (Parent == nullptr){
						return gcnew List<Result^>(current_size);
					}
					else{
						List<Result^>^ result = Parent->getResultVector(current_size + 1);
						result->Add(gcnew Result(Link, Parent->Position, Position));
						return result;
					}
				}
			public:
				List<Result^>^ ToResultVector(){
					return getResultVector(0);
				}
				Step^ Move(TNode next_pos, TData link_data){
					Step^step = gcnew Step();
					step->Parent = this;
					step->Position = next_pos;
					step->Link = link_data;
					return step;
				}
			};
			Dictionary<TNode, Dictionary<TNode, TData>^>^ Grid;
			Queue<Step^>^ OpenSteps = gcnew Queue<Step^>();
			HashSet<TNode>^ ProcessedSteps = gcnew HashSet<TNode>();
			Func<TNode, Boolean>^ Target;

			List<Result^>^ processStep()
			{
				Step^ current = OpenSteps->Dequeue();
				if (Target(current->Position)){
					return current->ToResultVector();
				}
				if (ProcessedSteps->Add(current->Position))
				{
					Dictionary<TNode, TData>^ availableLinks;
					if (Grid->TryGetValue(current->Position, availableLinks))
					{
						for each(KeyValuePair<TNode, TData> link in availableLinks)
						{
							OpenSteps->Enqueue(current->Move(link.Key, link.Value));
						}
					}
				}
				return nullptr;
			}
		public:
			static List<Result^>^ FindPath(TNode from_node, Func<TNode, Boolean>^ to_node, Dictionary < TNode, Dictionary<TNode, TData>^>^ navigation_grid){
				Pathing<TNode, TData>^ p = gcnew Pathing<TNode, TData>();
				p->Grid = navigation_grid;
				p->Target = to_node;
				p->OpenSteps->Enqueue(Step::CreateFirst(from_node));
				while (p->OpenSteps->Count > 0)
				{
					List<Result^>^ finalVector = p->processStep();
					if (finalVector != nullptr)
						return finalVector;
				}
				return nullptr;
			}
			static array<TData>^ FindLinks(TNode from_node, Func<TNode, Boolean>^ to_node, Dictionary<TNode, Dictionary<TNode, TData>^>^ navigation_grid){
				List<Result^>^ full_path = FindPath(from_node, to_node, navigation_grid);
				if (full_path == nullptr){
					return nullptr;
				}
				array<TData>^ result = gcnew array<TData>(full_path->Count);
				for (int i = 0; i < full_path->Count; i++){
					result[i] = full_path[i]->LinkData;
				}
				return result;
			}
		private:
			ref class FindPathToNodeScope{
				TNode Target;
			public:
				FindPathToNodeScope(TNode trg){
					Target = trg;
				}
				bool IsTarget(TNode current){
					return EqualityComparer<TNode>::Default->Equals(current, Target);
				}
			};
		public:
			static List<Result^>^ FindPath(TNode from_node, TNode to_node, Dictionary<TNode, Dictionary<TNode, TData>^>^ navigation_grid){
				return FindPath(
					from_node,
					gcnew Func<TNode, Boolean>(gcnew FindPathToNodeScope(to_node), &FindPathToNodeScope::IsTarget),
					navigation_grid);
			}
			static array<TData>^ FindLinks(TNode from_node, TNode to_node, Dictionary<TNode, Dictionary<TNode, TData>^>^ navigation_grid){
				return FindLinks(
					from_node,
					gcnew Func<TNode, Boolean>(gcnew FindPathToNodeScope(to_node), &FindPathToNodeScope::IsTarget),
					navigation_grid);
			}
		};
}