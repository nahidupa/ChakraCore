//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// This is a special use doubly linked list whose iterators are always valid
// no matter what modifications are made to the list during iteration. The
// iterators rely on deleted nodes still having valid next and prev references
// to the nodes they used to be next to before being removed. They also rely
// on nodes being recycler allocated so that active iterators do not need to
// be tracked by the list and updated when deletes occur. Finally, well
// defined iteration order is maintained by only allowing new items to be
// appended to the end of the list. This allows an iterator to validate
// itself by seeking backwards until it finds a node whose previous's next
// still points to it.
//
// The intended use of this list is to track insertion order for items added
// to ES6 Map and Set objects. If a more general use if found for this data
// structure please generalize it and consider moving it to Common\DataStructures.

namespace Js
{
    template <typename TData>
    class MapOrSetDataNode
    {
    private:
        template <typename TData>
        friend class MapOrSetDataList;

        Field(MapOrSetDataNode<TData>*) next;
        Field(MapOrSetDataNode<TData>*) prev;

        MapOrSetDataNode(TData& data) : data(data), next(nullptr), prev(nullptr) { }

    public:
        Field(TData) data;
    };

    template <typename TData>
    class MapOrSetDataList
    {
    private:
        Field(MapOrSetDataNode<TData>*) first;
        Field(MapOrSetDataNode<TData>*) last;

    public:
        MapOrSetDataList(VirtualTableInfoCtorEnum) {};
        MapOrSetDataList() : first(nullptr), last(nullptr) { }

        class Iterator
        {
            Field(MapOrSetDataList<TData>*) list;
            Field(MapOrSetDataNode<TData>*) current;
        public:
            Iterator() : list(nullptr), current(nullptr) { }
            Iterator(MapOrSetDataList<TData>* list) : list(list), current(nullptr) { }

            bool Next()
            {
                // Nodes can be deleted while iterating so validate current
                // and if it is not valid find last valid node by following
                // previous toward first.
                // Note: clear will simply null out first and last, but
                // not invalidated nodes, so we must also check to see that
                // first is not null

                if (list == nullptr || list->first == nullptr)
                {
                    // list is empty or was cleared during enumeration
                    list = nullptr;
                    current = nullptr;
                    return false;
                }

                if (current)
                {
                    while (current->prev && current->prev->next != current)
                    {
                        current = current->prev;
                    }

                    if (current->prev == nullptr && current != list->first)
                    {
                        current = list->first;

                        if (current != nullptr)
                        {
                            return true;
                        }

                        list = nullptr;
                        current = nullptr;
                        return false;
                    }

                    if (current->next == nullptr && current != list->last)
                    {
                        Assert(list->last == nullptr);
                        current = nullptr;
                    }
                }

                if (current != list->last)
                {
                    if (current == nullptr)
                    {
                        Assert(list->first != nullptr);
                        current = list->first;
                    }
                    else
                    {
                        current = current->next;
                    }
                    return true;
                }

                list = nullptr;
                current = nullptr;
                return false;
            }

            const TData& Current() const
            {
                return current->data;
            }

            Field(MapOrSetDataNode<TData>*) CurrentNode() const
            {
                return current;
            }
        };

        void Clear()
        {
            first = nullptr;
            last = nullptr;
        }

        MapOrSetDataNode<TData>* Append(TData& data, Recycler* recycler)
        {
            // Must allocate with the recycler so that iterators can continue
            // to point to removed nodes.  That is, cannot delete nodes when
            // they are removed.  Reference counting would also work.
            MapOrSetDataNode<TData>* newNode = RecyclerNew(recycler, MapOrSetDataNode<TData>, data);

            if (last == nullptr)
            {
                Assert(first == nullptr);
                first = newNode;
                last = newNode;
            }
            else
            {
                newNode->prev = last;
                last->next = newNode;
                last = newNode;
            }

            return newNode;
        }

        void Remove(MapOrSetDataNode<TData>* node)
        {
            // Cannot delete the node itself, nor change its next and prev pointers!
            // Otherwise active iterators may break. Iterators depend on nodes existing
            // until garbage collector picks them up.
            auto next = node->next;
            auto prev = node->prev;

            if (next)
            {
                next->prev = prev;
            }
            else
            {
                Assert(last == node);
                last = prev;
            }

            if (prev)
            {
                prev->next = next;
            }
            else
            {
                Assert(first == node);
                first = next;
            }
        }

        Iterator GetIterator()
        {
            return Iterator(this);
        }
    };


    template <typename TData>
    class DelayedDeleteMapOrSetDataList : MapOrSetDataList<TData>
    {
        Field(SList<TData>*) deletedList;

    };
}
