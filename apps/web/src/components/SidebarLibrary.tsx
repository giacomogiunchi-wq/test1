import { useLibraryStore } from '../state/libraryStore';
import { useGraphStore } from '../state/graphStore';
import { UNIT_FLOW } from '../lib/units';

export function SidebarLibrary() {
  const { items } = useLibraryStore();
  const addNode = useGraphStore((state) => state.addNodeFromTemplate);

  return (
    <aside className="w-64 border-r border-slate-200 bg-white p-4 text-sm dark:border-slate-700 dark:bg-slate-900">
      <h2 className="text-lg font-semibold">Libreria</h2>
      <p className="mt-2 text-xs text-slate-500">Trascina o clicca per aggiungere nodi. Portate in {UNIT_FLOW}.</p>
      <ul className="mt-4 space-y-2">
        {items.map((item) => (
          <li key={item.id}>
            <button
              type="button"
              className="w-full rounded-md border border-slate-200 px-3 py-2 text-left shadow-sm hover:border-primary hover:text-primary dark:border-slate-700"
              onClick={() => addNode({ ...item.template, type: item.type, label: item.label })}
            >
              <div className="font-medium">{item.label}</div>
              <div className="text-xs uppercase text-slate-400">{item.type}</div>
            </button>
          </li>
        ))}
      </ul>
    </aside>
  );
}
