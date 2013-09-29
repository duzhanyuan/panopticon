#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#define BOOST_RESULT_OF_USE_DECLTYPE
#include <boost/iterator/transform_iterator.hpp>

#pragma once

namespace po
{
	template<typename N, typename E>
	struct graph;

	template<typename X>
	class descriptor
	{
	public:
		static descriptor construct(const X &x) { descriptor ret; ret.m_ptr = &x; return ret; }

		descriptor(void) : m_ptr(0) {}
		descriptor(const descriptor &v) : m_ptr(v.m_ptr) {}
		descriptor &operator=(const descriptor &v) { if(!(*this == v)) m_ptr = v.m_ptr; return *this; }
		bool operator!=(const descriptor &v) const { return !(*this == v); }
		bool operator==(const descriptor &v) const { return (m_ptr == 0 && v.m_ptr == 0) ||
																												(m_ptr && v.m_ptr && *m_ptr == *v.m_ptr); }

	private:
		const X&operator*(void) const { return *m_ptr; }
		const X *m_ptr;

		template<typename,typename>
		friend struct po::graph;
		friend struct std::hash<descriptor<X>>;
	};
}

namespace std
{
	template<typename X>
	struct hash<po::descriptor<X>>
	{
		size_t operator()(const po::descriptor<X> &d) const
		{
			return reinterpret_cast<const size_t>(d.m_ptr);
		}
	};
}

namespace boost
{
	template<typename N, typename E>
	struct graph_traits<po::graph<N,E>>
	{
		// Graph concept
		using vertex_descriptor = po::descriptor<N>;
		using edge_descriptor = po::descriptor<E>;
		using directed_category = directed_tag;
		using edge_parallel_category = disallow_parallel_edge_tag;
		struct traversal_category : public vertex_list_graph_tag, edge_list_graph_tag, bidirectional_graph_tag {};

		// VertexListGraph concept
		using vertex_iterator = typename boost::transform_iterator<std::function<po::descriptor<N>(const N&)>, typename std::unordered_set<N>::const_iterator>;
		using vertices_size_type = typename std::unordered_set<N>::size_type;

		// EdgeListGraph concept
		using edge_iterator = typename boost::transform_iterator<std::function<po::descriptor<E>(const E&)>, typename std::unordered_set<E>::const_iterator>;
		using edges_size_type = typename std::unordered_set<E>::size_type;

		// IncidenceGraph concept
		using out_edge_iterator = typename boost::transform_iterator<std::function<po::descriptor<E>(const std::pair<po::descriptor<N>,po::descriptor<E>>&)>, typename std::unordered_multimap<po::descriptor<N>,po::descriptor<E>>::const_iterator>;
		using degree_size_type = typename std::unordered_multimap<po::descriptor<N>,po::descriptor<E>>::size_type;

		// BidirectionalGraph concept
		using in_edge_iterator = typename boost::transform_iterator<std::function<po::descriptor<E>(const std::pair<po::descriptor<N>,po::descriptor<E>>&)>, typename std::unordered_multimap<po::descriptor<N>,po::descriptor<E>>::const_iterator>;
	};
}

namespace po
{
	template<typename N, typename E>
	struct graph
	{
		using node_iterator = typename boost::transform_iterator<std::function<po::descriptor<N>(const N&)>, typename std::unordered_set<N>::const_iterator>;
		using edge_iterator = typename boost::transform_iterator<std::function<po::descriptor<E>(const E&)>, typename std::unordered_set<E>::const_iterator>;
		using out_edge_iterator = typename boost::graph_traits<graph<N,E>>::out_edge_iterator;
		using in_edge_iterator = typename boost::graph_traits<graph<N,E>>::in_edge_iterator;
		using size_type = size_t;

		graph(void) : m_nodes(), m_edges(), m_neighbors(), m_forward(), m_backward() {}

		std::pair<edge_iterator, edge_iterator>
		edges(void) const
		{
			return std::make_pair(boost::make_transform_iterator(m_edges.cbegin(),std::function<po::descriptor<E>(const E&)>([](const E &e) { return po::descriptor<E>::construct(e); })),
														boost::make_transform_iterator(m_edges.cend(),std::function<po::descriptor<E>(const E&)>([](const E &e) { return po::descriptor<E>::construct(e); })));
		}

		std::pair<node_iterator, node_iterator>
		nodes(void) const
		{
			return std::make_pair(boost::make_transform_iterator(m_nodes.begin(),std::function<po::descriptor<N>(const N&)>([](const N &n) { return po::descriptor<N>::construct(n); })),
														boost::make_transform_iterator(m_nodes.end(),std::function<po::descriptor<N>(const N&)>([](const N &n) { return po::descriptor<N>::construct(n); })));
		}

		typename boost::graph_traits<graph<N,E>>::edges_size_type
		num_edges(void) const
		{
			return m_edges.size();
		}

		typename boost::graph_traits<graph<N,E>>::vertices_size_type
		num_nodes(void) const
		{
			return m_nodes.size();
		}

		po::descriptor<N>
		source(const po::descriptor<E> &e) const
		{
			return m_neighbors.find(e)->second.first;
		}

		po::descriptor<N>
		target(const po::descriptor<E> &e) const
		{
			return m_neighbors.find(e)->second.second;
		}

		std::pair<out_edge_iterator,out_edge_iterator>
		out_edges(po::descriptor<N> d) const
		{
			auto p = m_forward.equal_range(d);
			std::function<po::descriptor<E>(const std::pair<po::descriptor<N>,po::descriptor<E>>&)> fn = [](const std::pair<po::descriptor<N>,po::descriptor<E>> &e) { return e.second; };

			return std::make_pair(boost::make_transform_iterator(p.first,fn),
														boost::make_transform_iterator(p.second,fn));
		}

		std::pair<in_edge_iterator,in_edge_iterator>
		in_edges(po::descriptor<N> d) const
		{
			auto p = m_backward.equal_range(d);
			std::function<po::descriptor<E>(const std::pair<po::descriptor<N>,po::descriptor<E>>&)> fn = [](const std::pair<po::descriptor<N>,po::descriptor<E>> &e) { return e.second; };

			return std::make_pair(boost::make_transform_iterator(p.first,fn),
														boost::make_transform_iterator(p.second,fn));
		}

		po::descriptor<N>
		insert_node(const N &n)
		{
			return po::descriptor<N>::construct(*m_nodes.insert(n).second);
		}

		po::descriptor<E>
		insert_edge(const E &e, po::descriptor<N> from, po::descriptor<N> to)
		{
			assert(m_nodes.count(*from) && m_nodes.count(*to));

			m_edges.insert(e);
			m_forward.insert(from,po::descriptor<E>::construct(e));
			m_backward.insert(to,po::descriptor<E>::construct(e));
		}

		void remove_edge(po::descriptor<E> e)
		{
			auto del = [](std::pair<po::descriptor<N>,po::descriptor<E>> &p, std::unordered_multimap<po::descriptor<N>,po::descriptor<E>> &m)
			{
				auto is = m.equal_range(p.first);
				auto i = is.first;

				while(i != is.second)
					if(*i == p)
						m.erase(i);
					else
						++i;
			};
			auto n = m_neighbors[e];

			del(std::make_pair(e,n->second.first),m_backward);
			del(std::make_pair(e,n->second.second),m_forward);
			m_neighbors.erase(n);
			m_edges.erase(*e);
		}

		void remove_node(po::descriptor<N> n)
		{
			for(auto i = m_forward.find(n); i != m_forward.end();)
				remove_edge(i->second);

			for(auto i = m_backward.find(n); i != m_backward.end();)
				remove_edge(i->second);

			m_nodes.erase(*n);
		}

		node_iterator
		find_node(const N& n) const
		{
			return boost::make_transform_iterator(m_nodes.find(*n),std::function<po::descriptor<N>(const N&)>([](const N &n) { return po::descriptor<N>::construct(n); }));
		}

		edge_iterator
		find_edge(const E &e) const
		{
			return boost::make_transform_iterator(m_edges.find(*e),std::function<po::descriptor<E>(const E&)>([](const E &n) { return po::descriptor<E>::construct(n); }));
		}

		const N &get_node(po::descriptor<N> &n) const
		{
			assert(m_nodes.count(*n));
			return *n;
		}

		N &get_node(po::descriptor<N> &n)
		{
			assert(m_nodes.count(*n));
			return *n;
		}

		const E &get_edge(po::descriptor<E> &e) const
		{
			assert(m_edges.count(*e));
			return *e;
		}

		E &get_edge(po::descriptor<E> &e)
		{
			assert(m_edges.count(*e));
			return *e;
		}

	private:
		std::unordered_set<N> m_nodes;
		std::unordered_set<E> m_edges;
		std::unordered_map<po::descriptor<E>,std::pair<po::descriptor<N>,po::descriptor<N>>> m_neighbors;
		std::unordered_multimap<po::descriptor<N>,po::descriptor<E>> m_forward;
		std::unordered_multimap<po::descriptor<N>,po::descriptor<E>> m_backward;
	};

	template<typename N, typename E>
	std::pair<typename boost::graph_traits<graph<N,E>>::vertex_iterator,
						typename boost::graph_traits<graph<N,E>>::vertex_iterator>
	vertices(const graph<N,E> &g)
	{
		return g.nodes();
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::vertices_size_type
	num_vertices(const graph<N,E> &g)
	{
		return g.num_nodes();
	}

	template<typename N, typename E>
	std::pair<typename boost::graph_traits<graph<N,E>>::edge_iterator,
						typename boost::graph_traits<graph<N,E>>::edge_iterator>
	edges(const graph<N,E> &g)
	{
		return g.edges();
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::edges_size_type
	num_edges(const graph<N,E> &g)
	{
		return g.num_edges();
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::vertex_descriptor
	source(const typename boost::graph_traits<graph<N,E>>::edge_descriptor &e, const graph<N,E> &g)
	{
		return g.source(e);
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::vertex_descriptor
	target(const typename boost::graph_traits<graph<N,E>>::edge_descriptor &e, const graph<N,E> &g)
	{
		return g.target(e);
	}

	template<typename N, typename E>
	std::pair<typename boost::graph_traits<graph<N,E>>::out_edge_iterator,
						typename boost::graph_traits<graph<N,E>>::out_edge_iterator>
	out_edges(const typename boost::graph_traits<graph<N,E>>::vertex_descriptor &v, const graph<N,E> &g)
	{
		return g.out_edges(v);
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::degree_size_type
	out_degree(const typename boost::graph_traits<graph<N,E>>::vertex_descriptor &v, const graph<N,E> &g)
	{
		auto p = g.out_edges(v);
		return std::distance(p.first,p.second);
	}

	template<typename N, typename E>
	std::pair<typename boost::graph_traits<graph<N,E>>::in_edge_iterator,
						typename boost::graph_traits<graph<N,E>>::in_edge_iterator>
	in_edges(const typename boost::graph_traits<graph<N,E>>::vertex_descriptor &v, const graph<N,E> &g)
	{
		return g.in_edges(v);
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::degree_size_type
	in_degree(const typename boost::graph_traits<graph<N,E>>::vertex_descriptor &v, const graph<N,E> &g)
	{
		auto p = g.in_edges(v);
		return std::distance(p.first,p.second);
	}

	template<typename N, typename E>
	typename boost::graph_traits<graph<N,E>>::degree_size_type
	degree(const typename boost::graph_traits<graph<N,E>>::vertex_descriptor &v, const graph<N,E> &g)
	{
		return out_degree(v,g) + in_degree(v,g);
	}
}
