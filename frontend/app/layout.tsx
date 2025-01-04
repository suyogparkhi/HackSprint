import type { Metadata } from 'next'
import './globals.css'
import Link from 'next/link'
import { BarChart2, Bot, LineChart, Settings, AlertTriangle } from 'lucide-react'

export const metadata: Metadata = {
  title: 'AI Trading Platform',
  description: 'Advanced trading platform with AI capabilities',
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode
}>) {
  return (
    <html lang="en">
      <body>
        <div className="flex h-screen">
          {/* Sidebar */}
          <div className="w-64 bg-[#f7f6f3] dark:bg-gray-900 border-r border-gray-200 dark:border-gray-800">
            <div className="h-16 flex items-center px-6 border-b border-gray-200 dark:border-gray-800">
              <h1 className="text-xl font-bold text-[#1e3a8a] dark:text-blue-500">AI Trading Platform</h1>
            </div>
            <nav className="p-4 space-y-2">
              <Link href="/trading" className="flex items-center space-x-3 px-3 py-2 rounded-lg hover:bg-white/60 dark:hover:bg-gray-800">
                <BarChart2 className="w-5 h-5" />
                <span>Trading</span>
              </Link>
              <Link href="/ai-agent" className="flex items-center space-x-3 px-3 py-2 rounded-lg hover:bg-white/60 dark:hover:bg-gray-800">
                <Bot className="w-5 h-5" />
                <span>AI Agent</span>
              </Link>
              <Link href="/analytics" className="flex items-center space-x-3 px-3 py-2 rounded-lg hover:bg-white/60 dark:hover:bg-gray-800">
                <LineChart className="w-5 h-5" />
                <span>Analytics</span>
              </Link>
              <Link href="/risk" className="flex items-center space-x-3 px-3 py-2 rounded-lg hover:bg-white/60 dark:hover:bg-gray-800">
                <AlertTriangle className="w-5 h-5" />
                <span>Risk Management</span>
              </Link>
              <Link href="/settings" className="flex items-center space-x-3 px-3 py-2 rounded-lg hover:bg-white/60 dark:hover:bg-gray-800">
                <Settings className="w-5 h-5" />
                <span>Settings</span>
              </Link>
            </nav>
          </div>
          
          {/* Main content */}
          <div className="flex-1 overflow-auto">
            <div className="h-16 border-b border-gray-200 dark:border-gray-800 flex items-center justify-between px-6 bg-[#f7f6f3] dark:bg-gray-900">
              <div className="text-sm text-gray-500">24h Volume: $1.2M</div>
            </div>
            <main className="p-6">
              {children}
            </main>
          </div>
        </div>
      </body>
    </html>
  )
}
